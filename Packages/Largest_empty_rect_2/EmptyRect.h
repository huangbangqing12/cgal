#include <fstream>
#include <set.h>
#include <list.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Point_2.h>
#include <CGAL/predicates_on_points_2.h>
#include <CGAL/Segment_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/IO/leda_window.h>
#include <CGAL/leda_real.h>

enum point_type{reg,bot_right,bot_left,top_left,top_right};

using namespace CGAL;

template<class NT>
class Largest_Empty_Rect {
  typedef pair<pair<NT,NT>,pair<NT,NT> >    Bbox;
  typedef CGAL::Cartesian<NT>               Repr;
  typedef CGAL::Point_2<Repr>               Point_2;
  typedef CGAL::Vector_2<Repr>              Vector_2; 
  typedef CGAL::Polygon_traits_2<Repr>      Traits;
  typedef std::list<Point_2>                Container;
  typedef CGAL::Polygon_2<Traits,Container> Polygon;

private:

  struct y_ptr_bigger;
  struct x_ptr_bigger;

  class point_data {
  public:
    NT x;
    NT y;
    set<point_data *,y_ptr_bigger> *right_tent;
    set<point_data *,y_ptr_bigger> *left_tent;
    point_type type;

    point_data(NT x_inp,NT y_inp);
    point_data(NT x_inp,NT y_inp,set<point_data *,y_ptr_bigger> *r_tent,set<point_data *,y_ptr_bigger> *l_tent);
    point_data(NT x_inp,NT y_inp,set<point_data *,y_ptr_bigger> *r_tent,set<point_data *,y_ptr_bigger> *l_tent,point_type i_type);
    point_data(const point_data &other) : x(other.x),y(other.y),right_tent(other.right_tent),left_tent(other.left_tent) {}
    ~point_data() {delete right_tent;delete left_tent;}
    bool x_smaller(point_data *second) {return(x < second->x || x == second->x && y < second->y);}
    bool y_smaller(point_data *second) {return(y < second->y || y == second->y && x < second->x);}
    bool x_bigger(point_data *second) {return(x > second->x || x == second->x && y > second->y);}
    bool y_bigger(point_data *second) {return(y > second->y || y == second->y && x > second->x);}
  }; 

  struct y_ptr_bigger
  {
    bool operator()(const point_data *a,const point_data *b) const
    {
      return(a->y < b->y || a->y == b->y && a->x < b->x);
    }
  };

  struct x_ptr_bigger
  {
    bool operator()(const point_data *a,const point_data *b) const
    {
      return(a->x < b->x || a->x == b->x && a->y < b->y);
    }
  };

  struct point_bigger
  {
    bool operator()(const Point_2 &a,const Point_2 &b) const
    {
      return(a.x() < b.x() || a.x() == b.x() && a.y() < b.y());
    }
  };

  typedef set<point_data *,x_ptr_bigger> Point_data_set_of_x;
  typedef set<point_data *,y_ptr_bigger> Point_data_set_of_y;

  Point_data_set_of_x x_sorted;
  Point_data_set_of_y y_sorted;
  set<Point_2,point_bigger> public_set;
  NT min_x,min_y,max_x,max_y,min_x2,min_y2,max_x2,max_y2;
  NT biggest_rect_x0,biggest_rect_y0,biggest_rect_x1,biggest_rect_y1,biggest_rect_size;
  Polygon *polygon;

  Bbox get_biggest_rect();
  void phase_1();
  void phase_1_on_x();
  void phase_1_on_y();
  void phase_2_on_bot();
  void phase_2_on_top();
  void phase_2_on_left();
  void phase_2_on_right();
  void phase_2();
  void phase_3();
  void check_for_bigger(NT x0,NT y0,NT x1,NT y1);
  void copy_y_point_data_to_list(list<point_data *> &point_data_list);
  void copy_x_point_data_to_list(list<point_data *> &point_data_list);
  void tent(point_data *first,point_data *second);
  void tent(point_data *first,point_data *second,point_data *third);
  void get_next_for_top(list<point_data *>::iterator &iter,list<point_data *>::iterator &beyond);
  void get_prev_for_top(list<point_data *>::iterator &iter);
  void get_next_for_bot(list<point_data *>::iterator &iter,list<point_data *>::iterator &beyond);
  void get_prev_for_bot(list<point_data *>::iterator &iter);
  void get_next_for_bot(Point_data_set_of_y::iterator &iter);
  void Largest_Empty_Rect<NT>::get_next_for_bot(Point_data_set_of_y::iterator &iter,Point_data_set_of_y::iterator &last);
  void get_prev_for_bot(Point_data_set_of_y::iterator &iter);
  void get_next_for_left(list<point_data *>::iterator &iter,list<point_data *>::iterator &beyond);
  void get_prev_for_left(list<point_data *>::iterator &iter);
  void get_next_for_right(list<point_data *>::iterator &iter,list<point_data *>::iterator &beyond);
  void get_prev_for_right(list<point_data *>::iterator &iter);
  void determine_first_two_iters(Point_data_set_of_y::iterator &iter1,Point_data_set_of_y::iterator &iter2,Point_data_set_of_y::iterator &iter3,bool &first_iter_is_right,bool &second_iter_is_right,bool &third_iter_is_right);
  void determine_next_iter(Point_data_set_of_y::iterator &iter,Point_data_set_of_y::iterator &right_iter,Point_data_set_of_y::iterator &left_iter,Point_data_set_of_y::const_iterator right_iter_end,Point_data_set_of_y::const_iterator left_iter_end,bool &iter_is_right,bool &exist);
  void calls_for_tents(Point_data_set_of_y::iterator iter1,Point_data_set_of_y::iterator iter2);
  void calls_for_tents(Point_data_set_of_y::iterator iter1,Point_data_set_of_y::iterator iter2,Point_data_set_of_y::iterator iter3);
  void phase_2_update_y_sorted_list();
  void phase_3_check_for_bigger(Point_data_set_of_y::iterator iter,Point_data_set_of_y::iterator iter1,Point_data_set_of_y::iterator iter2,Point_data_set_of_y::iterator iter3,bool first_iter_is_right,bool second_iter_is_right,bool third_iter_is_right);
  void empty_tents();

public:
  typedef set<Point_2,point_bigger>::const_iterator const_iterator;
  // ctor
  Largest_Empty_Rect(Bbox &b);
  // ctor
  Largest_Empty_Rect(Polygon &inp_polygon);
  // add a point to data
  void add_point(Point_2 p,point_type i_type = reg);
  // remove a point from data
  bool remove_point(Point_2 p);
  // retrieve biggest rectangle
  Bbox get_largest_empty_rectangle();
  // clear data(remove points)
  void clear();
  // get a begin iterator to points
  const_iterator begin();
  // get a after-the-end iterator to points
  const_iterator end();

  // get a list of the current points
  void get_list_of_points(list<pair<NT,NT> > &points_list);
  // get bounding box
  Bbox get_bounding_box();
  // dtor
  ~Largest_Empty_Rect();
};

template<class NT>
Largest_Empty_Rect<NT>::~Largest_Empty_Rect()
{
  for(Point_data_set_of_x::iterator iter = x_sorted.begin();iter != x_sorted.end();++iter)
    delete(*iter);
}

template<class NT>
Largest_Empty_Rect<NT>::point_data::point_data(NT x_inp,NT y_inp) : x(x_inp),y(y_inp),type(reg)
{
  right_tent = 0;
  left_tent = 0;
}

template<class NT>
Largest_Empty_Rect<NT>::point_data::point_data(NT x_inp,NT y_inp,Point_data_set_of_y *r_tent,Point_data_set_of_y *l_tent) : x(x_inp),y(y_inp),right_tent(r_tent),left_tent(l_tent),type(reg) {}

template<class NT>
Largest_Empty_Rect<NT>::point_data::point_data(NT x_inp,NT y_inp,Point_data_set_of_y *r_tent,Point_data_set_of_y *l_tent,point_type i_type) : x(x_inp),y(y_inp),right_tent(r_tent),left_tent(l_tent),type(i_type) {}

template<class NT>
void Largest_Empty_Rect<NT>::get_list_of_points(list<pair<NT,NT> > &points_list)
{
  Point_data_set_of_x::const_iterator last_point = x_sorted.end(),iter = x_sorted.begin();
  --last_point;
  --last_point;
  ++iter;
  ++iter;

  while(iter != last_point) {
    points_list.push_back(pair<NT,NT>((*iter)->x,(*iter)->y));
    ++iter;
  }
}

template<class NT>
void Largest_Empty_Rect<NT>::add_point(Point_2 p,point_type i_type)
{
  NT x = p.x(),y = p.y();
  Point_data_set_of_y *right_tent = new Point_data_set_of_y;
  Point_data_set_of_y *left_tent = new Point_data_set_of_y;
  point_data *po = new point_data(x,y,right_tent,left_tent,i_type);

  x_sorted.insert(po);
  y_sorted.insert(po);

  // insert to public point set
  Point_2 public_p(x,y);
  public_set.insert(public_p);
}

template<class NT>
bool Largest_Empty_Rect<NT>::remove_point(Point_2 p)
{
  NT x = p.x(),y = p.y();
  point_data *po = new point_data(x,y);
  Point_data_set_of_x::iterator iter1 = x_sorted.find(po),iter2 = y_sorted.find(po);

  // point does not exist or a corner point
  if(iter1 == x_sorted.end() || iter1->type != reg)
    return(false);

  delete(iter->right_tent);
  delete(iter->left_tent);

  x_sorted.erase(iter1);
  y_sorted.erase(iter2);

  // remove from public point set
  Point_2 public_p(x,y);
  public_set.erase(public_p);

  return(true);
}

template<class NT>
void Largest_Empty_Rect<NT>::check_for_bigger(NT x0,NT y0,NT x1,NT y1)
{
  bool do_check = true;

  if(polygon) {
    NT bw = x1 - x0;
    NT bh = y1 - y0;
    NT bx = x0 + bw/2;
    NT by = y0 + bh/2;

    Point_2 center(bx,by);
    do_check = polygon->has_on_bounded_side(center);
  }

  // check if the rectangle represented by the parameters is bigger than the current one
  if(do_check && abs(x1 - x0) * abs(y1 - y0) > biggest_rect_size) {
    biggest_rect_size = abs(x1 - x0) * abs(y1 - y0);
    biggest_rect_x0 = x0;
    biggest_rect_y0 = y0;
    biggest_rect_x1 = x1;
    biggest_rect_y1 = y1;
  }
}

template<class NT>
void Largest_Empty_Rect<NT>::phase_1_on_x()
{
  Point_data_set_of_x::const_iterator iter = x_sorted.begin(),last_iter = x_sorted.end(),prev_iter = iter;
  ++iter;

  // filter false points
  while((*iter)->type == top_right || (*iter)->type == top_left) {
    ++iter;
    ++prev_iter;
  }

  // traverse over all possibilities for finding a bigger empty rectangle
  // rectangles here touch the top and the buttom of the bounding box  
  while(iter != last_iter) {
    if((*iter)->type != top_right && (*iter)->type != top_left) {// filter false points
      check_for_bigger((*prev_iter)->x,min_y,(*iter)->x,max_y);
      prev_iter = iter;
    }
    ++iter;
  }
}

template<class NT>
void Largest_Empty_Rect<NT>::phase_1_on_y()
{
  Point_data_set_of_y::const_iterator iter = y_sorted.begin(),last_iter = y_sorted.end(),prev_iter = iter;
  ++iter;

  // filter false points
  while((*iter)->type == bot_right || (*iter)->type == top_right) {
    ++iter;
    ++prev_iter;
  }

  // traverse over all possibilities for finding a bigger empty rectangle
  // rectangles here touch the left and the right of the bounding box  
  while(iter != last_iter) {
    if((*iter)->type != bot_right && (*iter)->type != top_right) {// filter false points
      check_for_bigger(min_x,(*prev_iter)->y,max_x,(*iter)->y);
      prev_iter = iter;
    }
    ++iter;
  }
}

template<class NT>
void Largest_Empty_Rect<NT>::phase_1()
{
  phase_1_on_x();
  phase_1_on_y();
}

template<class NT>
void Largest_Empty_Rect<NT>::copy_x_point_data_to_list(list<point_data *> &point_data_list)
{
  for(Point_data_set_of_x::const_iterator iter = x_sorted.begin();iter != x_sorted.end();++iter)
    point_data_list.push_back(*iter);
}

template<class NT>
void Largest_Empty_Rect<NT>::copy_y_point_data_to_list(list<point_data *> &point_data_list)
{
  for(Point_data_set_of_y::const_iterator iter = y_sorted.begin();
      iter != (Point_data_set_of_y::const_iterator)y_sorted.end();
      ++iter)
    point_data_list.push_back(*iter);
}

template<class NT>
void Largest_Empty_Rect<NT>::tent(point_data *first,point_data *second)
{
  if(first->y_smaller(second))
    /* if(first->y < second->y)*/
    first->right_tent->insert(second);
  else
    second->left_tent->insert(first);
}

template<class NT>
void Largest_Empty_Rect<NT>::tent(point_data *first,point_data *second,point_data *third)
{
  first->right_tent->insert(second);
  third->left_tent->insert(second);
  if(first->y_smaller(third))
    /* if(first->y < third->y) */
    first->right_tent->insert(third);
  else
    third->left_tent->insert(first);
}

template<class NT>
void Largest_Empty_Rect<NT>::get_next_for_top(list<point_data *>::iterator &iter,list<point_data *>::iterator &beyond)
{
  while(iter != beyond && ((*iter)->type == bot_right || (*iter)->type == bot_left))
    ++iter;
}

template<class NT>
void Largest_Empty_Rect<NT>::get_prev_for_top(list<point_data *>::iterator &iter)
{
  while((*iter)->type == bot_right || (*iter)->type == bot_left)
    --iter;
}

template<class NT>
void Largest_Empty_Rect<NT>::get_next_for_bot(list<point_data *>::iterator &iter,list<point_data *>::iterator &beyond)
{
  while(iter != beyond && ((*iter)->type == top_left || (*iter)->type == top_right))
    ++iter;
}

template<class NT>
void Largest_Empty_Rect<NT>::get_prev_for_bot(list<point_data *>::iterator &iter)
{
  while((*iter)->type == top_left || (*iter)->type == top_right)
    --iter;
}

template<class NT>
void Largest_Empty_Rect<NT>::get_next_for_bot(Point_data_set_of_y::iterator &iter)
{
  while((*iter)->type == top_left || (*iter)->type == top_right)
    ++iter;
}

template<class NT>
void Largest_Empty_Rect<NT>::get_next_for_bot(Point_data_set_of_y::iterator &iter,Point_data_set_of_y::iterator &last)
{
  while(iter != last && ((*iter)->type == top_left || (*iter)->type == top_right))
    ++iter;
}

template<class NT>
void Largest_Empty_Rect<NT>::get_prev_for_bot(Point_data_set_of_y::iterator &iter)
{
  while((*iter)->type == top_left || (*iter)->type == top_right)
    --iter;
}

template<class NT>
void Largest_Empty_Rect<NT>::get_next_for_left(list<point_data *>::iterator &iter,list<point_data *>::iterator &beyond)
{
  while(iter != beyond && ((*iter)->type == bot_right || (*iter)->type == top_right))
    ++iter;
}

template<class NT>
void Largest_Empty_Rect<NT>::get_prev_for_left(list<point_data *>::iterator &iter)
{
  while((*iter)->type == bot_right || (*iter)->type == top_right)
    --iter;
}

template<class NT>
void Largest_Empty_Rect<NT>::get_next_for_right(list<point_data *>::iterator &iter,list<point_data *>::iterator &beyond)
{
  while(iter != beyond && ((*iter)->type == bot_left || (*iter)->type == top_left))
    ++iter;
}

template<class NT>
void Largest_Empty_Rect<NT>::get_prev_for_right(list<point_data *>::iterator &iter)
{
  while((*iter)->type == bot_left || (*iter)->type == top_left)
    --iter;
}

template<class NT>
void Largest_Empty_Rect<NT>::phase_2_on_bot()
{
  list<point_data *> point_data_list;
  copy_x_point_data_to_list(point_data_list);
  list<point_data *>::iterator iter1 = point_data_list.begin(),iter2,iter3,first_iter,beyond = point_data_list.end();
  int points_removed = 0,size = point_data_list.size();

  get_next_for_bot(iter1,beyond);
  first_iter = iter1;
  iter2 = iter1;
  ++iter2;
  get_next_for_bot(iter2,beyond);
  iter3 = iter2;
  ++iter3;
  get_next_for_bot(iter3,beyond);

  while(size - 4 > points_removed && iter3 != point_data_list.end()) {
    if((*iter1)->y_smaller(*iter2) && (*iter2)->y_bigger(*iter3)) {
      // Rectangles in phase 2 should be ignored for polygon
      if(!polygon)
        check_for_bigger((*iter1)->x,min_y,(*iter3)->x,(*iter2)->y);
      tent(*iter1,*iter2,*iter3);
      ++points_removed;
      point_data_list.erase(iter2);
      if(iter1 != first_iter) { // move back
        iter2 = iter1;
        --iter1;
        get_prev_for_bot(iter1);
      } else {
        iter2 = iter3;
        ++iter3;
        get_next_for_bot(iter3,beyond);
      }
    } else {// iter3 can't be last
      iter1 = iter2;
      iter2 = iter3;
      ++iter3;
      get_next_for_bot(iter3,beyond);
    }
  }
}


template<class NT>
void Largest_Empty_Rect<NT>::phase_2_on_top()
{
  list<point_data *> point_data_list;
  copy_x_point_data_to_list(point_data_list);
  list<point_data *>::iterator iter1 = point_data_list.begin(),iter2,iter3,first_iter,beyond = point_data_list.end();
  int points_removed = 0,size = point_data_list.size();

  get_next_for_top(iter1,beyond);
  iter2 = iter1;
  first_iter = iter1;
  ++iter2;
  get_next_for_top(iter2,beyond);
  iter3 = iter2;
  ++iter3;
  get_next_for_top(iter3,beyond);

  while(size - 4 > points_removed && iter3 != point_data_list.end()) {
    if((*iter1)->y_bigger(*iter2) && (*iter2)->y_smaller(*iter3)) {
      check_for_bigger((*iter1)->x,max_y,(*iter3)->x,(*iter2)->y);
      ++points_removed;
      point_data_list.erase(iter2);
      if(iter1 != first_iter) { // move back
        iter2 = iter1;
        --iter1;
        get_prev_for_top(iter1);
      } else {
        iter2 = iter3;
        ++iter3;
        get_next_for_top(iter3,beyond);
      }
    } else {// iter3 can't be last
      iter1 = iter2;
      iter2 = iter3;
      ++iter3;
      get_next_for_top(iter3,beyond);
    }
  }
}

template<class NT>
void Largest_Empty_Rect<NT>::phase_2_on_left()
{
  list<point_data *> point_data_list;
  copy_y_point_data_to_list(point_data_list);
  list<point_data *>::iterator iter1 = point_data_list.begin(),iter2,iter3,first_iter,beyond = point_data_list.end();
  int points_removed = 0,size = point_data_list.size();

  get_next_for_left(iter1,beyond);
  first_iter = iter1;
  iter2 = iter1;
  ++iter2;
  get_next_for_left(iter2,beyond);
  iter3 = iter2;
  ++iter3;
  get_next_for_left(iter3,beyond);

  while(size - 4 > points_removed && iter3 != point_data_list.end()) {
    if((*iter1)->x_smaller(*iter2) && (*iter2)->x_bigger(*iter3)) {
      check_for_bigger(min_x,(*iter1)->y,(*iter2)->x,(*iter3)->y);
      ++points_removed;
      point_data_list.erase(iter2);
      if(iter1 != first_iter) { // move back
        iter2 = iter1;
        --iter1;
        get_prev_for_left(iter1);
      } else {
       iter2 = iter3;
        ++iter3;
        get_next_for_left(iter3,beyond);
      }
    } else {// iter3 can't be last
      iter1 = iter2;
      iter2 = iter3;
      ++iter3;
      get_next_for_left(iter3,beyond);
    }
  }
}

template<class NT>
void Largest_Empty_Rect<NT>::phase_2_on_right()
{
  list<point_data *> point_data_list;
  copy_y_point_data_to_list(point_data_list);
  list<point_data *>::iterator iter1 = point_data_list.begin(),iter2,iter3,first_iter,beyond = point_data_list.end();
  int points_removed = 0,size = point_data_list.size();

  get_next_for_right(iter1,beyond);
  first_iter = iter1;
  iter2 = iter1;
  ++iter2;
  get_next_for_right(iter2,beyond);
  iter3 = iter2;
  ++iter3;
  get_next_for_right(iter3,beyond);

  while(size - 4 > points_removed && iter3 != point_data_list.end()) {
    if((*iter1)->x_bigger(*iter2) && (*iter2)->x_smaller(*iter3)) {
      check_for_bigger((*iter2)->x,(*iter1)->y,max_x,(*iter3)->y);
      ++points_removed;
      point_data_list.erase(iter2);
      if(iter1 != first_iter) { // move back
        iter2 = iter1;
        --iter1;
        get_prev_for_right(iter1);
      } else {
        iter2 = iter3;
        ++iter3;
        get_next_for_right(iter3,beyond);
      }
    } else {// iter3 can't be last
      iter1 = iter2;
      iter2 = iter3;
      ++iter3;
      get_next_for_right(iter3,beyond);
    }
  }
}

template<class NT>
void Largest_Empty_Rect<NT>::phase_2()
{
  // Rectangles in phase 2 should be ignored for polygon
  if(!polygon) {
    phase_2_on_top();
    phase_2_on_left();
    phase_2_on_right();
  }

  // Done only for building tents for phase 3
  phase_2_on_bot();
}

template<class NT>
Largest_Empty_Rect<NT>::Bbox Largest_Empty_Rect<NT>::get_biggest_rect()
{
  return(Bbox(pair<NT,NT>(biggest_rect_x0,biggest_rect_y0),pair<NT,NT>(biggest_rect_x1,biggest_rect_y1)));
}

template<class NT>
void 
Largest_Empty_Rect<NT>::determine_next_iter(Point_data_set_of_y::iterator &iter,
					    Point_data_set_of_y::iterator &right_iter,
					    Point_data_set_of_y::iterator &left_iter,
					    Point_data_set_of_y::const_iterator right_iter_end,
					    Point_data_set_of_y::const_iterator left_iter_end,
					    bool &iter_is_right,
					    bool &exist)
{
  if((Point_data_set_of_y::const_iterator)right_iter != right_iter_end) {
    if((Point_data_set_of_y::const_iterator)left_iter != left_iter_end) {
      if((*right_iter)->y_smaller(*left_iter)) {
	/* if((*right_iter)->y < (*left_iter)->y) { */
        iter = right_iter;
        iter_is_right = true;
        ++right_iter;
      } else {
        iter = left_iter;
        iter_is_right = false;
        ++left_iter;
      }
    } else {
      iter = right_iter;
      iter_is_right = true;
      ++right_iter;
    }
  } else { 
    if((Point_data_set_of_y::const_iterator)left_iter != left_iter_end) {
      iter = left_iter;
      iter_is_right = false;
      ++left_iter;
     } else
      exist = false;
  }
}

template<class NT>
void Largest_Empty_Rect<NT>::phase_3_check_for_bigger(Point_data_set_of_y::iterator iter,Point_data_set_of_y::iterator iter1,Point_data_set_of_y::iterator iter2,Point_data_set_of_y::iterator iter3,bool first_iter_is_right,bool second_iter_is_right,bool third_iter_is_right)
{
  if(first_iter_is_right) {
    if(!second_iter_is_right)
      check_for_bigger((*iter2)->x,(*iter)->y,(*iter1)->x,(*iter3)->y);
  } else
    if(second_iter_is_right)
      check_for_bigger((*iter1)->x,(*iter)->y,(*iter2)->x,(*iter3)->y);
}

template<class NT>
void Largest_Empty_Rect<NT>::calls_for_tents(Point_data_set_of_y::iterator iter1,Point_data_set_of_y::iterator iter2,Point_data_set_of_y::iterator iter3)
{
  bool first_is_right_to_second = (*iter1)->x_smaller(*iter2);
  bool second_is_right_to_third = (*iter2)->x_smaller(*iter3);

  if(first_is_right_to_second) {
    if(second_is_right_to_third) {
      tent(*iter1,*iter2);
      tent(*iter2,*iter3);
    } else {
      tent(*iter1,*iter3,*iter2);
    }
  } else {
    if(second_is_right_to_third) {
      tent(*iter2,*iter3,*iter1);
    }
    else {
      tent(*iter2,*iter1);
      tent(*iter3,*iter2);
    }
  }
}

template<class NT>
void Largest_Empty_Rect<NT>::calls_for_tents(Point_data_set_of_y::iterator iter1,Point_data_set_of_y::iterator iter2)
{
  if((*iter1)->x_smaller(*iter2))
    /* if((*iter1)->x < (*iter2)->x) */
    tent(*iter1,*iter2);
  else
    tent(*iter2,*iter1);
}

template<class NT>
void Largest_Empty_Rect<NT>::phase_3()
{
  bool first_iter_is_right,second_iter_is_right,third_iter_is_right,first_exist,second_exist,third_exist;
  Point_data_set_of_y::iterator iter,last_iter = y_sorted.end();
  Point_data_set_of_y::iterator iter1,iter2,iter3,right_iter,left_iter,last = last_iter;

  --last_iter;
  --last_iter;
  for(iter = y_sorted.begin();iter != last_iter;++iter) {
    get_next_for_bot(iter,last);
    if(iter == last)
      return;
    first_exist = true;
    second_exist = true;
    third_exist = true;

    right_iter = (*iter)->right_tent->begin();
    left_iter = (*iter)->left_tent->begin();
    determine_next_iter(iter1,right_iter,left_iter,(*iter)->right_tent->end(),(*iter)->left_tent->end(),first_iter_is_right,first_exist);
    determine_next_iter(iter2,right_iter,left_iter,(*iter)->right_tent->end(),(*iter)->left_tent->end(),second_iter_is_right,second_exist);
    determine_next_iter(iter3,right_iter,left_iter,(*iter)->right_tent->end(),(*iter)->left_tent->end(),third_iter_is_right,third_exist);
    bool had_three = false;

    while(third_exist) {
      had_three = true;
      phase_3_check_for_bigger(iter,iter1,iter2,iter3,first_iter_is_right,second_iter_is_right,third_iter_is_right);
      calls_for_tents(iter1,iter2,iter3);
      determine_first_two_iters(iter1,iter2,iter3,first_iter_is_right,second_iter_is_right,third_iter_is_right);
      determine_next_iter(iter3,right_iter,left_iter,(*iter)->right_tent->end(),(*iter)->left_tent->end(),third_iter_is_right,third_exist);
    }

    if(!had_three && second_exist)
      calls_for_tents(iter1,iter2);
  }
}

template<class NT>
void Largest_Empty_Rect<NT>::empty_tents()
{
  for(Point_data_set_of_x::const_iterator iter = x_sorted.begin();iter != x_sorted.end();++iter) {
    (*iter)->right_tent->clear();
    (*iter)->left_tent->clear();
  }
}

template<class NT>
Largest_Empty_Rect<NT>::Bbox Largest_Empty_Rect<NT>::get_largest_empty_rectangle()
{
  if(x_sorted.size() == 4)
    return(get_bounding_box());

  biggest_rect_size = 0;

  // Rectangles in phase 1 should be ignored for polygon
  if(!polygon)
    phase_1();

  phase_2();
  phase_3();
  Bbox b = get_biggest_rect();
  empty_tents();

  return(b);
}

template<class NT>
Largest_Empty_Rect<NT>::Bbox Largest_Empty_Rect<NT>::get_bounding_box()
{
  return(Bbox(pair<NT,NT>(min_x,min_y),pair<NT,NT>(max_x,max_y)));
}

template<class NT>
Largest_Empty_Rect<NT>::Largest_Empty_Rect(Polygon &inp_polygon)
{
  polygon = new Polygon(inp_polygon);

  // determine extreme values of bounding box
  min_x2 = min_x = polygon->left_vertex()->x();
  min_y2 = min_y = polygon->bottom_vertex()->y();
  max_x2 = max_x = polygon->right_vertex()->x();
  max_y2 = max_y = polygon->top_vertex()->y();

  // add extreme points
  add_point(Point_2(min_x - 0.000001,min_y - 0.000001),bot_left);
  add_point(Point_2(max_x + 0.000001,min_y2 - 0.000001),bot_right);
  add_point(Point_2(min_x2 - 0.000001,max_y + 0.000001),top_left);
  add_point(Point_2(max_x2 + 0.000001,max_y2 + 0.000001),top_right);

  // insert the polygon 
  Polygon::Vertex_iterator it = polygon->vertices_begin();
  Polygon::Vertex_iterator next = it;

  Point_2 p,q;
  Point_2 p0 = *it;

  while(it != polygon->vertices_end()) {
    add_point(*it);
    ++next;
    if(next == polygon->vertices_end())
      q = p0;
    else
      q = *next;
    p = *it;

    // add some points on the segment 
    Vector_2 v = (q - p)/6;
    for(int j = 1; j < 6; j++) {
      add_point(p + j * v);
    }    

    ++it;
  }
}

template<class NT>
Largest_Empty_Rect<NT>::Largest_Empty_Rect(Bbox &b)
{
  polygon = NULL;

  NT x0 = b.first.first,y0 = b.first.second,x1 = b.second.first, y1 = b.second.second;
  // determine extreme values of bounding box
  min_x2 = min_x = ((x0 < x1) ? x0 : x1);
  min_y2 = min_y = ((y0 < y1) ? y0 : y1);
  max_x2 = max_x = ((x0 > x1) ? x0 : x1);
  max_y2 = max_y = ((y0 > y1) ? y0 : y1);

  // add extreme points
  add_point(Point_2(min_x - 0.000001,min_y - 0.000001),bot_left);
  add_point(Point_2(max_x + 0.000001,min_y2 - 0.000001),bot_right);
  add_point(Point_2(min_x2 - 0.000001,max_y + 0.000001),top_left);
  add_point(Point_2(max_x2 + 0.000001,max_y2 + 0.000001),top_right);
}


template<class NT>
Largest_Empty_Rect<NT>::const_iterator Largest_Empty_Rect<NT>::begin()
{
  return(public_set.begin());
}

template<class NT>
Largest_Empty_Rect<NT>::const_iterator Largest_Empty_Rect<NT>::end()
{
  return(public_set.end());
}


template<class NT>
void Largest_Empty_Rect<NT>::clear()
{
  for(Point_data_set_of_x::iterator iter = x_sorted.begin();iter != x_sorted.end();++iter)
    delete(*iter);

  x_sorted.clear();
  y_sorted.clear();

  add_point(Point_2(min_x - 0.000001,min_y - 0.000001),bot_left);
  add_point(Point_2(max_x + 0.000001,min_y2 - 0.000001),bot_right);
  add_point(Point_2(min_x2 - 0.000001,max_y + 0.000001),top_left);
  add_point(Point_2(max_x2 + 0.000001,max_y2 + 0.000001),top_right);
}


template<class NT>
void Largest_Empty_Rect<NT>::determine_first_two_iters(Point_data_set_of_y::iterator &iter1,Point_data_set_of_y::iterator &iter2,Point_data_set_of_y::iterator &iter3,bool &first_iter_is_right,bool &second_iter_is_right,bool &third_iter_is_right)
{
  if(first_iter_is_right) {
    if(second_iter_is_right) {
      iter1 = iter2;
      iter2 = iter3;
      first_iter_is_right = second_iter_is_right;
      second_iter_is_right = third_iter_is_right;
    } else {
      if(third_iter_is_right) {
        iter1 = iter2;
        iter2 = iter3;
        first_iter_is_right = second_iter_is_right;
        second_iter_is_right = third_iter_is_right;  
      } else {
        iter2 = iter3;
        second_iter_is_right = third_iter_is_right;  
      }
    }
  } else {
    if(second_iter_is_right) {
      if(third_iter_is_right) {
        iter2 = iter3;
        second_iter_is_right = third_iter_is_right;  
      } else {
        iter1 = iter2;
        iter2 = iter3;
        first_iter_is_right = second_iter_is_right;
        second_iter_is_right = third_iter_is_right;  
      }
    } else {
      iter1 = iter2;
      iter2 = iter3;
      first_iter_is_right = second_iter_is_right;
      second_iter_is_right = third_iter_is_right;  
    }
  }
}

