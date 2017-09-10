#include <vector>
#include <memory>
#include <map>
#include <thread>
#include <imgui.h>
#include <imgui-SFML.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics.hpp>

#include "logger.cpp"
#include "../MapgenConfig.h"
#include "SelbaWard/SelbaWard.hpp"
#include "infoWindow.cpp"
#include "objectsWindow.cpp"

class Application {
  std::vector<sf::ConvexShape> polygons;
  std::vector<sf::ConvexShape> infoPolygons;
  std::vector<sf::Vertex> verticies;
  sf::Color bgColor;
  AppLog log;
  MapGenerator* mapgen;
  std::thread generator;
  sw::ProgressBar progressBar;
  sf::Font sffont;
  sf::RenderWindow* window;
  bool isIncreasing{ true };

  int relax = 0;
  int seed;
  int octaves;
  float freq;
  int nPoints;
  bool borders = false;
  bool sites = false;
  bool edges = false;
  bool info = false;
  bool heights = false;
  bool flat = false;
  bool hum = false;
  bool simplifyRivers;
  int t = 0;
  float color[3] = { 0.12, 0.12, 0.12 };
  bool showUI = true;
  bool getScreenshot = false;

public:
  Application() {
    seed = std::clock();
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("./font.ttf", 15.0f);

    window = new sf::RenderWindow(sf::VideoMode::getDesktopMode(), "", sf::Style::Default, settings);
    window->setVerticalSyncEnabled(true);
    ImGui::SFML::Init(*window);
    char windowTitle[255] = "MapGen";

    window->setTitle(windowTitle);
    window->resetGLStates(); // call it if you only draw ImGui. Otherwise not needed.

    initMapGen();
    // mapgen.setSeed(/*111629613*/ 81238299);
    generator = std::thread([&](){});
    regen();

    bgColor.r = static_cast<sf::Uint8>(color[0] * 255.f);
    bgColor.g = static_cast<sf::Uint8>(color[1] * 255.f);
    bgColor.b = static_cast<sf::Uint8>(color[2] * 255.f);

    progressBar.setShowBackgroundAndFrame(true);
    progressBar.setSize(sf::Vector2f(400, 10));
    progressBar.setPosition((sf::Vector2f(window->getSize()) - progressBar.getSize()) / 2.f);

    sffont.loadFromFile("./font.ttf");

    log.AddLog("Welcome to Mapgen\n");
  }

  void regen() {
    generator.join();
    generator = std::thread([&](){
        mapgen->update();
        seed = mapgen->getSeed();
        relax = mapgen->getRelax();
        updateVisuals();
      });
  }

  void initMapGen() {
    mapgen = new MapGenerator(window->getSize().x, window->getSize().y);
    octaves = mapgen->getOctaveCount();
    freq = mapgen->getFrequency();
    nPoints = mapgen->getPointCount();
    relax = mapgen->getRelax();
    simplifyRivers = mapgen->simpleRivers;
  }

  void processEvent(sf::Event event) {
      ImGui::SFML::ProcessEvent(event);

      switch (event.type)
        {
        case sf::Event::KeyPressed:
          switch (event.key.code)
            {
            case sf::Keyboard::R:
              mapgen->seed();
              log.AddLog("Update map\n");
              regen();
              break;
            case sf::Keyboard::Escape:
              window->close();
              break;
            case sf::Keyboard::U:
              showUI = !showUI;
              break;
            case sf::Keyboard::S:
              showUI = false;
              getScreenshot = true;
              break;
            }
          break;
        case sf::Event::Closed:
          window->close();
          break;
        case sf::Event::Resized:
          mapgen->setSize(window->getSize().x, window->getSize().y);
          log.AddLog("Update map\n");
          mapgen->update();
          updateVisuals();
          break;
        }
  }

  void fade() {
    sf::RectangleShape rectangle;
    rectangle.setSize(sf::Vector2f(window->getSize().x, window->getSize().y));
    auto color = sf::Color::Black;
    color.a = 150;
    rectangle.setFillColor(color);
    rectangle.setPosition(0, 0);
    window->draw(rectangle);
  }

  void drawLoading(sf::Clock* clock) {
          const float frame{ clock->restart().asSeconds() * 0.3f };
          const float target{ isIncreasing ? progressBar.getRatio() + frame : progressBar.getRatio() - frame };
          if (target < 0.f)
            isIncreasing = true;
          else if (target > 1.f)
            isIncreasing = false;
          progressBar.setRatio(target);
          sf::Text operation(mapgen->currentOperation, sffont);
          operation.setCharacterSize(20);
          operation.setColor(sf::Color::White);

          auto middle = (sf::Vector2f(window->getSize())) / 2.f;
          operation.setPosition(sf::Vector2f(middle.x - operation.getGlobalBounds().width/2.f, middle.y+25.f));

          window->draw(progressBar);

          sf::RectangleShape bg;
          bg.setSize(sf::Vector2f(420, 40));
          bg.setFillColor(sf::Color::Black);
          bg.setOutlineColor(sf::Color::White);
          bg.setOutlineThickness(1);
          middle = (sf::Vector2f(window->getSize()) - bg.getSize()) / 2.f;
          bg.setPosition(sf::Vector2f(middle.x, middle.y + 37.f));
          window->draw(bg);
          window->draw(operation);
          window->display();
  }

  void drawMainWindow() {
        ImGui::Begin("Mapgen"); // begin window
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Polygons: %zu", polygons.size());

        ImGui::Text("Window size: w:%d h:%d",
                    window->getSize().x,
                    window->getSize().y
                    );

        if (ImGui::Checkbox("Borders",&borders)) {
          updateVisuals();
        }
        ImGui::SameLine(100);
        ImGui::Checkbox("Sites",&sites);
        ImGui::SameLine(200);
        if(ImGui::Checkbox("Edges",&edges)){
            updateVisuals();
        }
        if(ImGui::Checkbox("Heights",&heights)){
          updateVisuals();
        }
        ImGui::SameLine(100);
        if(ImGui::Checkbox("Flat",&flat)){
          updateVisuals();
        }
        ImGui::SameLine(200);
        if(ImGui::Checkbox("Info",&info)) {
          infoPolygons.clear();
          updateVisuals();
        }

        if(ImGui::Checkbox("Humidity",&hum)) {
          infoPolygons.clear();
          updateVisuals();
        }
        ImGui::SameLine(100);
        if(ImGui::Checkbox("Simplify rivers",&simplifyRivers)) {
          mapgen->simpleRivers = simplifyRivers;
          regen();
        }

        if (ImGui::InputInt("Seed", &seed)) {
          mapgen->setSeed(seed);
          log.AddLog("Update map\n");
          regen();
        }

        const char* templates[] = {"basic", "archipelago", "new"};
        if (ImGui::Combo("Map template", &t, templates, 3)) {
          mapgen->setMapTemplate(templates[t]);
          regen();
        }

        if (ImGui::Button("Random")) {
          mapgen->seed();
          regen();
        }
        ImGui::SameLine(100);
        if (ImGui::Button("Update")) {
          mapgen->forceUpdate();
          updateVisuals();
        }

        if (ImGui::SliderInt("Height octaves", &octaves, 1, 10)) {
          mapgen->setOctaveCount(octaves);
          regen();
        }

        if (ImGui::SliderFloat("Height freq", &freq, 0.001, 2.f)) {
          mapgen->setFrequency(freq);
          regen();
        }

        if (ImGui::InputInt("Points", &nPoints)) {
          if (nPoints < 5) {
            nPoints = 5;
          }
          mapgen->setPointCount(nPoints);
          regen();
        }

        // if (ImGui::Button("Relax")) {
        //   log.AddLog("Update map\n");
        //   mapgen.relax();
        //   updateVisuals();
        //   relax = mapgen.getRelax();
        // }
        // ImGui::SameLine(100);
        ImGui::Text("Relax iterations: %d", relax);
        if (ImGui::Button("+1000")) {
          nPoints+=1000;
          mapgen->setPointCount(nPoints);
          regen();
        }
        ImGui::SameLine(100);
        if (ImGui::Button("-1000")) {
          nPoints-=1000;
          mapgen->setPointCount(nPoints);
          regen();
        }

        ImGui::Text("\n[ESC] for exit\n[S] for save screenshot\n[R] for random map\n[U] toggle ui");

        ImGui::End(); // end window
  }

  void drawInfo() {
          sf::Vector2<float> pos = window->mapPixelToCoords(sf::Mouse::getPosition(*window));
          Region* currentRegion =  mapgen->getRegion(pos);
          // char p[100];
          // sprintf(p,"%p\n",currentRegion);
          // std::cout<<p<<std::flush;
          sf::ConvexShape selectedPolygon;

          infoWindow(window, currentRegion);

          infoPolygons.clear();

          PointList points = currentRegion->getPoints();
          selectedPolygon.setPointCount(int(points.size()));

          Cluster* cluster = currentRegion->cluster;

          int i = 0;
          for(std::vector<Region*>::iterator it=cluster->megaCluster->regions.begin() ; it < cluster->megaCluster->regions.end(); it++, i++) {

            Region* region = cluster->megaCluster->regions[i];
            sf::ConvexShape polygon;
            PointList points = region->getPoints();
            polygon.setPointCount(points.size());
            int n = 0;
            for(PointList::iterator it2=points.begin() ; it2 < points.end(); it2++, n++) {
              sf::Vector2<double>* p = points[n];
              polygon.setPoint(n, sf::Vector2f(p->x, p->y));
            }
            sf::Color col = sf::Color::Yellow;
            col.a = 20;
            polygon.setFillColor(col);
            polygon.setOutlineColor(col);
            polygon.setOutlineThickness(1);
            infoPolygons.push_back(polygon);
          }
          i = 0;
          for(std::vector<Region*>::iterator it=cluster->regions.begin() ; it < cluster->regions.end(); it++, i++) {

            Region* region = cluster->regions[i];
            sf::ConvexShape polygon;
            PointList points = region->getPoints();
            polygon.setPointCount(points.size());
            int n = 0;
            for(PointList::iterator it2=points.begin() ; it2 < points.end(); it2++, n++) {
              sf::Vector2<double>* p = points[n];
              polygon.setPoint(n, sf::Vector2f(p->x, p->y));
            }
            sf::Color col = sf::Color::Red;
            col.a = 50;
            polygon.setFillColor(col);
            polygon.setOutlineColor(col);
            polygon.setOutlineThickness(1);
            infoPolygons.push_back(polygon);
          }

          for (int pi = 0; pi < int(points.size()); pi++) {
            Point p = points[pi];
            selectedPolygon.setPoint(pi, sf::Vector2f(static_cast<float>(p->x), static_cast<float>(p->y)));
          }

          sf::CircleShape site(2.f);

          selectedPolygon.setFillColor(sf::Color::Transparent);
          selectedPolygon.setOutlineColor(sf::Color::Red);
          selectedPolygon.setOutlineThickness(2);
          site.setFillColor(sf::Color::Red);
          site.setPosition(static_cast<float>(currentRegion->site->x-1),static_cast<float>(currentRegion->site->y-1));

          i = 0;
          for(std::vector<sf::ConvexShape>::iterator it=infoPolygons.begin() ; it < infoPolygons.end(); it++, i++) {
            window->draw(infoPolygons[i]);
          }

          window->draw(selectedPolygon);
          window->draw(site);
  }

  void drawRivers() {
        int rn = 0;
        for (auto r : mapgen->rivers){
          PointList* rvr = r->points;
          sw::Spline river;
          river.setThickness(3);
          int i = 0;
          int c = rvr->size();
          for(PointList::iterator it=rvr->begin() ; it < rvr->end(); it++, i++) {
            Point p = (*rvr)[i];
            river.addVertex(i, {static_cast<float>(p->x), static_cast<float>(p->y)});
            float t = float(i)/c * 2.f;
            river.setThickness(i, t);
            if(rivers_selection_mask.size() >= mapgen->rivers.size() && rivers_selection_mask[rn]) {
              river.setColor(sf::Color( 255, 70, 0));
            } else {
              river.setColor(sf::Color( 46, 46, 76, float(i)/c * 255.f));
            }
          }
          river.setBezierInterpolation(); // enable Bezier spline
          river.setInterpolationSteps(10); // curvature resolution
          river.smoothHandles();
          river.update();
          window->draw(river);
          rn++;
        }
  }

  void drawMap() {
    int i = 0;
    for(std::vector<sf::ConvexShape>::iterator it=polygons.begin() ; it < polygons.end(); it++, i++) {
      window->draw(polygons[i]);
    }
  }

  void drawObjects() {
    auto op = objectsWindow(window, mapgen);

    int i = 0;
    for(std::vector<sf::ConvexShape>::iterator it=op.begin() ; it < op.end(); it++, i++) {
      window->draw(op[i]);
    }
  }

  void serve() {
    sf::Clock deltaClock;
    sf::Clock clock;

    bool faded = false;
    while (window->isOpen()) {
        sf::Event event;
        while (window->pollEvent(event)) {
          processEvent(event);
        }

        if (!mapgen->ready) {
          if (!faded) {
            fade();
            faded = true;
          }
          drawLoading(&clock);
          continue;
        }
        faded = false;

        ImGui::SFML::Update(*window, deltaClock.restart());

        window->clear(bgColor); // fill background with color

        drawMap();
        drawRivers();

        sf::Vector2u windowSize = window->getSize();
        sf::Text mark("Mapgen by Averrin", sffont);
        mark.setCharacterSize(15);
        mark.setColor(sf::Color::White);
        mark.setPosition(sf::Vector2f(windowSize.x-160, windowSize.y-25));
        window->draw(mark);

        if(showUI){
          drawObjects();
          drawMainWindow();
          log.Draw("Mapgen: Log", &info);

          if(info) {
            drawInfo();
          }

          if (sites) {
            for (auto v : verticies) {
              window->draw(&v, 1, sf::PrimitiveType::Points);
            }
          }
        }

        ImGui::SFML::Render(*window);
        window->display();
        if (getScreenshot) {
          sf::Texture texture;
          texture.create(windowSize.x, windowSize.y);
          texture.update(*window);
          sf::Image screenshot = texture.copyToImage();
          char s[100];
          sprintf(s, "%d.png", seed);
          screenshot.saveToFile(s);
          char l[255];
          sprintf(l, "Screenshot created: %s\n", s);
          log.AddLog(l);
          showUI = true;
          getScreenshot = false;
        }
    }
 
    generator.join();
    ImGui::SFML::Shutdown();
  }

  void updateVisuals() {
    log.AddLog("Update geometry\n");
    polygons.clear();
    verticies.clear();
    int i = 0;
    std::vector<Region*>* regions = mapgen->getRegions();
    polygons.reserve(regions->size());
    verticies.reserve(regions->size());
    for(std::vector<Region*>::iterator it=regions->begin() ; it < regions->end(); it++, i++) {

      Region* region = (*regions)[i];
      sf::ConvexShape polygon;
      PointList points = region->getPoints();
      polygon.setPointCount(points.size());
      int n = 0;
      for(PointList::iterator it2=points.begin() ; it2 < points.end(); it2++, n++) {
        sf::Vector2<double>* p = points[n];
        polygon.setPoint(n, sf::Vector2f(p->x, p->y));
      }

      sf::Color col(region->biom.color);
      if(!flat){
      int a = 255 * (region->getHeight(region->site)+1.6)/3;
      if (a > 255) {
        a = 255;
      }
      col.a = a;
      }
      polygon.setFillColor(col);
      if(edges) {
        polygon.setOutlineColor(sf::Color(100,100,100));
        polygon.setOutlineThickness(1);
      }
      if(borders) {
        if(region->border) {
          polygon.setOutlineColor(sf::Color(100,50,50));
          polygon.setOutlineThickness(1);
        }
      }
      if(heights) {
        sf::Color col(region->biom.color);
        col.r = 255 * (region->getHeight(region->site)+1.6)/3.2;
        col.a = 20 + 255 * (region->getHeight(region->site)+1.6)/3.2;
        col.b = col.b/3;
        col.g = col.g/3;
        polygon.setFillColor(col);
        color[2] = 1.f;
      }

      if(hum && region->humidity != 1) {
        sf::Color col(region->biom.color);
        col.b = 255 * region->humidity;
        col.a = 255 * region->humidity;
        col.r = col.b/3;
        col.g = col.g/3;
        polygon.setFillColor(col);
      }
      polygons.push_back(polygon);
      verticies.push_back(sf::Vertex(sf::Vector2f(region->site->x, region->site->y), sf::Color(100,100,100)));
    }

  }
};