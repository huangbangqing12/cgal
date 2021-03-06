// Copyright (c) 2002  Utrecht University (The Netherlands).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
//
// Author(s)     : Radu Ursu
// ============================================================================
//

#include <CGAL/basic.h>


#include "cgal_types.h"

#include <CGAL/IO/Qt_widget.h>
#include <CGAL/IO/Qt_widget_standard_toolbar.h>
#include <CGAL/IO/Qt_help_window.h>
#include <CGAL/IO/Qt_widget_layer.h>
#include "spatial_searching_toolbar.h"
#include <CGAL/IO/pixmaps/demoicon.xpm>

#include <qapplication.h>
#include <qmainwindow.h>
#include <qstatusbar.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qtoolbutton.h>
#include <qtoolbar.h>
#include <qfiledialog.h>
#include <qtimer.h>


//global flags and variables
int current_state;
std::vector<Point_2>                      vector_of_points;

const QString my_title_string("Spatial searching");


class Qt_layer_show_ch : public CGAL::Qt_widget_layer
{
public:

  Qt_layer_show_ch(){};


  void draw()
  {
    widget->lock();
    *widget << CGAL::PointSize(3);
    *widget << CGAL::GREEN;
    std::vector<Point_2>::iterator itp = vector_of_points.begin();
    while(itp!=vector_of_points.end())
    {
      *widget << (*itp++);
    }
    widget->unlock();
  };

};//end class

class MyWindow : public QMainWindow
{
  Q_OBJECT
public:
  MyWindow(int w, int h){
    widget = new CGAL::Qt_widget(this);
    setCentralWidget(widget);

    //create a timer for checking if somthing changed
    QTimer *timer = new QTimer( this );
    connect( timer, SIGNAL(timeout()),
           this, SLOT(timer_done()) );
    timer->start( 200, FALSE );

    // file menu
    QPopupMenu * file = new QPopupMenu( this );
    menuBar()->insertItem( "&File", file );
    file->insertItem("&New", this, SLOT(new_instance()), CTRL+Key_N);
    file->insertItem("New &Window", this, SLOT(new_window()), CTRL+Key_W);
    file->insertSeparator();
    file->insertItem("Print", widget, SLOT(print_to_ps()), CTRL+Key_P);
    file->insertSeparator();
    file->insertItem( "&Close", this, SLOT(close()), CTRL+Key_X );
    file->insertItem( "&Quit", qApp, SLOT( closeAllWindows() ), CTRL+Key_Q );

    // drawing menu
    QPopupMenu * draw = new QPopupMenu( this );
    menuBar()->insertItem( "&Draw", draw );
    draw->insertItem("&Generate points", this,
				SLOT(gen_points()), CTRL+Key_G );

    // computation menu
    QPopupMenu * computation = new QPopupMenu( this );
    menuBar()->insertItem( "&Computation", computation );
    computation->insertItem("&Neighbour Search", this,
				SLOT(n_search()), CTRL+Key_S );

    // help menu
    QPopupMenu * help = new QPopupMenu( this );
    menuBar()->insertItem( "&Help", help );
    help->insertItem("How To", this, SLOT(howto()), Key_F1);
    help->insertSeparator();
    help->insertItem("&About", this, SLOT(about()), CTRL+Key_A );
    help->insertItem("About &Qt", this, SLOT(aboutQt()) );

    //the standard toolbar
    stoolbar = new CGAL::Qt_widget_standard_toolbar (widget, this, "ST");
    //the new tools toolbar
    newtoolbar = new Tools_toolbar(widget, this, &vector_of_points);

    *widget << CGAL::LineWidth(2) << CGAL::BackgroundColor (CGAL::WHITE);

    resize(w,h);
    widget->set_window(-1, 1, -1, 1);
    widget->setMouseTracking(TRUE);

    //connect the widget to the main function that receives the objects
    connect(widget, SIGNAL(new_cgal_object(CGAL::Object)),
      this, SLOT(get_new_object(CGAL::Object)));

    //application flag stuff
    old_state = 0;

    //layers
    widget->attach(&testlayer);
  };

private:
  void something_changed(){current_state++;};

public slots:
  void new_instance()
  {
    widget->lock();
    vector_of_points.clear();
    widget->clear_history();
    widget->set_window(-1.1, 1.1, -1.1, 1.1);
		// set the Visible Area to the Interval
    widget->unlock();
    something_changed();
  }

private slots:
  void get_new_object(CGAL::Object obj)
  {
    Point_2 p;
    Iso_rectangle_2 ir;
    Circle_2 c;
    if(CGAL::assign(p,obj)) {
      vector_of_points.push_back(p);
      something_changed();
    } else if(CGAL::assign(ir, obj)){
      // Searching an exact range
      // using default value 0.0 for epsilon fuzziness paramater
      Fuzzy_box exact_range((ir.min)(), (ir.max)());

      typedef CGAL::Kd_tree<Traits> Tree;
      std::list<Point_2> l, res;
      CGAL::cpp11::copy_n(vector_of_points.begin(), vector_of_points.size(),
                  std::back_inserter(l));
      std::cout << "construct tree with " << l.size() << " points" << std::endl;
      Tree d(l.begin(), l.end());
      std::cout << "search" << std::endl;
      d.search( std::back_inserter( res ), exact_range);
      std::cout << "search done. found " << res.size() << " points" << std::endl;
      widget->redraw();
      widget->lock();
      *widget << CGAL::RED;
      *widget << ir;
      std::list<Point_2>::const_iterator it = res.begin();
      while(it!=res.end()){
        *widget << (*it);
        it++;
      }
      widget->unlock();

    } else if(CGAL::assign(c, obj)){
      // exact range searching using default value 0.0 for fuzziness paramater
      Fuzzy_circle exact_range(c.center(), std::sqrt(c.squared_radius()));

      typedef CGAL::Kd_tree<Traits> Tree;
      std::list<Point_2> l, res;
      CGAL::cpp11::copy_n(vector_of_points.begin(), vector_of_points.size(),
                  std::back_inserter(l));
      Tree d(l.begin(), l.end());
      d.search( std::back_inserter( res ), exact_range);
      widget->redraw();
      widget->lock();
      *widget << CGAL::RED;
      *widget << c;
      std::list<Point_2>::const_iterator it = res.begin();
      while(it!=res.end()){
        *widget << (*it);
        it++;
      }
      widget->unlock();
    }
  };

  void about()
  {
    QMessageBox::about( this, my_title_string,
		"This is a demo for Spatial Searching\n"
  		"Copyright CGAL @2003");
  };

  void aboutQt()
  {
    QMessageBox::aboutQt( this, my_title_string );
  }

  void howto(){
    QString home;
    home = "help/index.html";
    CGAL::Qt_help_window *help = new
      CGAL::Qt_help_window(home, ".", 0, "help viewer");
    help->resize(400, 400);
    help->setCaption("Demo HowTo");
    help->show();
  }

  void new_window(){
    MyWindow *ed = new MyWindow(500, 500);
    ed->setCaption("Layer");
    ed->widget->clear_history();
    ed->widget->set_window(-1.1, 1.1, -1.1, 1.1);
    ed->show();
    something_changed();
  }

  void timer_done()
  {
    if(old_state!=current_state){
      widget->redraw();
      old_state = current_state;
    }
  }

  void gen_points()
  {
    widget->clear_history();
    widget->set_window(-1.1, 1.1, -1.1, 1.1);
		// set the Visible Area to the Interval

    // send resizeEvent only on show.
    CGAL::Random_points_in_disc_2<Point_2> g(1);
    for(int count=0; count<200; count++) {
      vector_of_points.push_back(*g++);
    }
    something_changed();
  }

  void n_search(){

    typedef CGAL::Kd_tree<Traits> Tree;
    std::list<Point_2> l;
    CGAL::cpp11::copy_n(vector_of_points.begin(), vector_of_points.size(),
		std::back_inserter(l));
    //    Tree d(vector_of_points.begin(), vector_of_points.end(), tr);
    Neighbour_search::Tree d(l.begin(), l.end());

    const int query_point_number=30;
    CGAL::Random_points_in_square_2<Point_2,Creator> h( 1.0);
    std::vector<Point_2> query_points;
    CGAL::cpp11::copy_n( h, query_point_number, std::back_inserter(query_points));

    std::vector<Neighbour_search::Point_with_transformed_distance>
      nearest_neighbour;
    for (int i=0; i < query_point_number; i++) {

     Neighbour_search N(d, query_points[i]);
     std::copy(N.begin(), N.end(), std::back_inserter(nearest_neighbour));
    }
    widget->lock();
    for (int j=0; j < query_point_number; j++) {
      *widget << CGAL::RED;
      *widget << Segment_2(query_points[j], (nearest_neighbour[j].first));
      *widget << CGAL::YELLOW;
      *widget << query_points[j];
    }
    widget->unlock();
  }


private:
  CGAL::Qt_widget        *widget;
  CGAL::Qt_widget_standard_toolbar
                         *stoolbar;
  Tools_toolbar          *newtoolbar;
  int                    old_state;
  Qt_layer_show_ch       testlayer;
};

#include "spatial_searching.moc"


int
main(int argc, char **argv)
{
  QApplication app( argc, argv );
  MyWindow widget(500,500); // physical window size
  app.setMainWidget(&widget);
  widget.setCaption(my_title_string);
  widget.setMouseTracking(TRUE);
#if !defined (__POWERPC__)
  QPixmap cgal_icon = QPixmap((const char**)demoicon_xpm);
  widget.setIcon(cgal_icon);
#endif
  widget.show();
  current_state = -1;
  return app.exec();
}

