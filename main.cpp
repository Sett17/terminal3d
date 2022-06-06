#include <chrono>
#include <fstream>
#include <iostream>
#include <math.h>
#include <ncurses.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>

#define MAX 2147483647

#define PI 3.14159265

int FOV = 500;
int FAR_PLANE = 15;

int width, height;
int center_x = 0;
int center_y = 0;
int size = 0;

class Dot
{
private:
  /* data */
public:
  double x, y, z;
  int screenX = -1;
  int screenY = -1;
  double screenScale = 1;
  double rotation = 0;
  Dot()
  {
    x = MAX;
    y = MAX;
    z = MAX;
  }
  Dot(double x, double y, double z, int scale)
  {
    this->x = x * scale;
    this->y = y * scale;
    this->z = z * scale;
  };
  void project()
  {
    screenScale = FOV / (FOV + z);

    double rX = x * cos(rotation) - z * sin(rotation);

    screenX = (int)(rX * screenScale);
    screenY = (int)(-y * screenScale);
    // screenX = (int)(rX);
    // screenY = (int)(rY);
    if (screenX == 0 && screenY == 0)
    {
      // printw("warning %f %f %f\n", x, y, z);
    }
  };
  void draw()
  {
    this->project();
    // mvprintw(screenY + 25, screenX + 25, "@");
    mvprintw(screenY + center_y, (screenX + center_x) * 2, "@");
  }
  void print()
  {
    this->project();
    printw("%f, %f, %f, %d, %d, %f\n", x, y, z, screenX, screenY, screenScale);
  }
};

class Line
{
private:
  /* data */
public:
  Dot a, b;
  Line()
  {
    a = Dot();
    b = Dot();
  }
  Line(Dot a, Dot b)
  {
    this->a = a;
    this->b = b;
  }
  void draw()
  {
    a.project();
    b.project();

    // Xiaolin Wu's line algorithm
    int x0, y0, x1, y1;
    x0 = a.screenX;
    y0 = a.screenY;
    x1 = b.screenX;
    y1 = b.screenY;
    bool steep = abs(y1 - y0) > abs(x1 - x0);
    mvprintw(10, 2, "steep: %d", steep);
    if (steep)
    {
      // swapping x and y
      std::swap(x0, y0);
      std::swap(x1, y1);
    }
    if (x0 > x1)
    {
      // swapping 0 and 1
      int z = x0;
      x0 = x1;
      x1 = z;
      z = y0;
      y0 = y1;
      y1 = z;
    }

    int dx = x1 - x0;
    int dy = y1 - y0;

    double gradient;
    if (dx == 0)
    {
      // vertical line
      gradient = 1;
    }
    else
    {
      gradient = (double)dy / (double)dx;
    }

    // handle first endpoint
    int xend = x0;
    double yend = y0 + gradient * (xend - x0);
    double xgap = 0;
    int xpxl1 = xend;
    int ypxl1 = floor(yend);
    double intery = yend + gradient;

    // handle second endpoint
    xend = x1;
    yend = y1 + gradient * (xend - x1);
    xgap = 0;
    int xpxl2 = xend;
    int ypxl2 = floor(yend);

    // main loop
    for (int x = xpxl1 + 1; x <= xpxl2 - 1; x++)
    {
      double brightness1 = 1 - (intery - floor(intery));
      char c1;
      if (brightness1 < 0.5)
      {
        c1 = '-';
      }
      else
      {
        c1 = '*';
      }

      if (!steep)
      {
        mvaddch(floor(intery) + center_y, (x + center_x) * 2, c1);
      }
      else
      {
        mvaddch((x + center_y), (floor(intery) + center_x) * 2, c1);
      }
      intery = intery + gradient;
    }

    a.draw();
    b.draw();
  }
};

class Face
{
private:
  /* data */
public:
  int vertcount;
  Dot *verts = new Dot[4];
  double rotation = 0;
  Face()
  {
    vertcount = 0;
  }
  Face(int vertcount, Dot *verts)
  {
    this->vertcount = vertcount;
    this->verts = verts;
  }
  void draw()
  {
    for (int i = 0; i < vertcount; i++)
    {
      verts[i].rotation = rotation;
    }
    for (int i = 0; i < vertcount - 1; i++)
    {
      Line(verts[i], verts[i + 1]).draw();
    }
    Line(verts[0], verts[vertcount - 1]).draw();
  }
};

int count_verts(char *filename)
{
  std::string line;
  std::ifstream file;
  int vertCount = 0;
  file.open(filename);

  if ((file.is_open()))
  {
    while (getline(file, line))
    {
      if (line.substr(0, 2) == "v ")
      {
        vertCount++;
      }
    }
  }
  file.close();
  return vertCount;
}

int count_faces(char *filename)
{
  std::string line;
  std::ifstream file;
  int faceCount = 0;
  file.open(filename);

  if ((file.is_open()))
  {
    while (getline(file, line))
    {
      if (line.substr(0, 2) == "f ")
      {
        faceCount++;
      }
    }
  }
  file.close();
  return faceCount;
}

void load_obj(char *filename, Dot *verts, int vertCount, Face *faces, int faceCount)
{
  std::string line;
  std::ifstream file;
  file.open(filename);

  if (file.is_open())
  {
    int vertIndex = 0;
    int faceIndex = 0;
    while (getline(file, line))
    {
      if (line.substr(0, 2) == "v ")
      {
        std::string rest = line.substr(2);

        double pos[3];

        std::string s = rest;
        std::string delim = " ";
        auto start = 0U;
        auto end = s.find(delim);
        pos[0] = std::stod(s.substr(start, end - start));
        start = end + delim.length();
        end = s.find(delim, start);
        pos[1] = std::stod(s.substr(start, end - start));
        start = end + delim.length();
        end = s.find(delim, start);
        pos[2] = std::stod(s.substr(start, end - start));
        start = end + delim.length();
        end = s.find(delim, start);

        verts[vertIndex] = Dot(pos[0], pos[1], pos[2], size);
        vertIndex++;
      }
      else if (line.substr(0, 2) == "f ")
      {
        std::string rest = line.substr(2);

        Dot *faceVerts = new Dot[4];
        int i = 0;

        std::string s = rest;
        std::string delim = " ";
        auto start = 0U;
        auto end = s.find(delim);
        while (end != std::string::npos)
        {
          std::string vertNumberStr = s.substr(start, end - start);
          start = end + delim.length();
          end = s.find(delim, start);
          int vertNumber = std::stoi(vertNumberStr) - 1;
          faceVerts[i] = verts[vertNumber];
          i++;
        }
        faces[faceIndex] = Face(i, faceVerts);
        faceIndex++;
      }
    }
    file.close();
  }
}

void draw_verts(Dot *verts, int vertCount)
{
  double rot = 0;
  while (rot < PI * 10)
  {
    rot += 0.0015;
    usleep(1000);
    erase();
    for (int i = 0; i < vertCount; i++)
    {
      verts[i].rotation = rot;
    }
    move(0, 0);
    printw("%f\n", rot);
    printw("%d\n", vertCount);
    for (int i = 0; i < vertCount; i++)
    {
      verts[i].draw();
    }
    mvprintw(center_y, center_x * 2, "X");
    refresh();
  }
}

void draw_obj(Face *faces, int faceCount)
{
  double rot = 0;
  while (rot < PI * 10)
  {
    rot += 0.0015;
    usleep(500);
    erase();
    for (int i = 0; i < faceCount; i++)
    {
      faces[i].rotation = rot;
    }
    move(0, 0);
    printw("%f\n", rot);
    printw("%d\n", faceCount);
    for (int i = 0; i < faceCount; i++)
    {
      faces[i].draw();
    }
    mvprintw(center_y, center_x * 2, "X");
    refresh();
  }
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cout << "Usage: " << argv[0] << " <.obj file>" << std::endl;
    return 1;
  }

  WINDOW *wdw = initscr(); /* Start curses mode       */
  curs_set(0);

  getmaxyx(wdw, height, width);
  center_x = width / 4;
  center_y = height / 2;
  if (center_x < center_y)
  {
    size = center_x;
  }
  else
  {
    size = center_y;
  }
  size /= 1.3;
  size = 20;

  printw("%d %d", width, height);
  move(1, 0);
  printw("%d %d", center_x, center_y);
  move(2, 0);
  mvprintw(center_y, center_x * 2, "X");
  move(3, 0);

  // Dot a = Dot(-15, 0, 0, 1);
  // Dot b = Dot(15, -25, 0, 1);
  // Line line = Line(a, b);
  // line.draw();
  // for (size_t i = 0; i < 50; i++)
  // {
  //   erase();
  //   b.y++;
  //   line = Line(a, b);
  //   line.draw();
  //   refresh();
  //   usleep(70000);
  // }
  // for (size_t i = 0; i < 45; i++)
  // {
  //   erase();
  //   a.x++;
  //   line = Line(a, b);
  //   line.draw();
  //   refresh();
  //   usleep(70000);
  // }

  int vertCount = count_verts(argv[1]);
  Dot *verts = new Dot[vertCount];
  int faceCount = count_faces(argv[1]);
  Face *faces = new Face[faceCount];
  load_obj(argv[1], verts, vertCount, faces, faceCount);
  draw_obj(faces, faceCount);

  getch();
  curs_set(1);
  endwin(); /* End curses mode      */

  return 0;
}