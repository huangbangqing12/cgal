// Copyright (c) 1999-2004,2006-2009   INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
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
// Author(s)     : Monique Teillaud <Monique.Teillaud@sophia.inria.fr>
//                 Sylvain Pion <Sylvain.Pion@sophia.inria.fr>
//                 Andreas Fabri <Andreas.Fabri@sophia.inria.fr>
//                 Nico Kruithof <Nico.Kruithof@sophia.inria.fr>
//                 Manuel Caroli <Manuel.Caroli@sophia.inria.fr>


#ifndef CGAL_PERIODIC_3_DELAUNAY_TRIANGULATION_3_H
#define CGAL_PERIODIC_3_DELAUNAY_TRIANGULATION_3_H

#include <CGAL/Periodic_3_triangulation_3.h>
#include <CGAL/spatial_sort.h>

// Needed by remove to fill the hole.
#include <CGAL/Periodic_3_triangulation_remove_traits_3.h>
#include <CGAL/Delaunay_triangulation_3.h>

CGAL_BEGIN_NAMESPACE

template < class Gt,
            class Tds = Triangulation_data_structure_3 <
              Triangulation_vertex_base_3<
		Gt, Periodic_3_triangulation_ds_vertex_base_3<>
	      >,
              Triangulation_cell_base_3<
                Gt, Periodic_3_triangulation_ds_cell_base_3<>
              >
            >
          >
class Periodic_3_Delaunay_triangulation_3 :
    public Periodic_3_triangulation_3<Gt,Tds>
{
  typedef Periodic_3_Delaunay_triangulation_3<Gt,Tds>          Self;
public:
  typedef Periodic_3_triangulation_3<Gt,Tds>                   Base;

public:
  /** @name Template parameter types */ //@{
  typedef Gt                                    Geometric_traits;
  typedef Tds                                   Triangulation_data_structure;
  //@}

  ///Compatibility typedef:
  typedef Geometric_traits                      Geom_traits;

  typedef typename Gt::Point_3                  Point;
  typedef typename Gt::Segment_3                Segment;
  typedef typename Gt::Triangle_3               Triangle;
  typedef typename Gt::Tetrahedron_3            Tetrahedron;

  typedef typename Base::Periodic_point         Periodic_point;
  typedef typename Base::Periodic_segment       Periodic_segment;
  typedef typename Base::Periodic_triangle      Periodic_triangle;
  typedef typename Base::Periodic_tetrahedron   Periodic_tetrahedron;

  typedef typename Base::Cell_handle            Cell_handle;
  typedef typename Base::Vertex_handle          Vertex_handle;

  typedef typename Base::Cell                   Cell;
  typedef typename Base::Vertex                 Vertex;
  typedef typename Base::Facet                  Facet;
  typedef typename Base::Edge                   Edge;

  typedef typename Base::Cell_circulator        Cell_circulator;
  typedef typename Base::Facet_circulator       Facet_circulator;
  typedef typename Base::Cell_iterator          Cell_iterator;
  typedef typename Base::Facet_iterator         Facet_iterator;
  typedef typename Base::Edge_iterator          Edge_iterator;
  typedef typename Base::Vertex_iterator        Vertex_iterator;

  typedef typename Base::All_cells_iterator     All_cells_iterator;
  typedef typename Base::All_facets_iterator    All_facets_iterator;
  typedef typename Base::All_edges_iterator     All_edges_iterator;
  typedef typename Base::All_vertices_iterator  All_vertices_iterator;

  typedef typename Base::Locate_type            Locate_type;
  typedef typename Base::Iterator_type          Iterator_type;

  typedef typename Base::Offset                 Offset;

  typedef typename Base::Iso_cuboid             Iso_cuboid;
  //@}

//TODO: check which usings are necessary!
#ifndef CGAL_CFG_USING_BASE_MEMBER_BUG_2
  using Base::cw;
  using Base::ccw;
  using Base::domain;
  using Base::geom_traits;
  using Base::int_to_off;
  using Base::number_of_sheets;
  using Base::number_of_vertices;
  using Base::number_of_facets;
  using Base::cells_begin;
  using Base::cells_end;
  using Base::vertices_begin;
  using Base::vertices_end;
  using Base::facets_begin;
  using Base::facets_end;
  using Base::tds;
  using Base::next_around_edge;
  using Base::vertex_triple_index;
  using Base::mirror_vertex;
  using Base::orientation;
  using Base::insert_dummy_points;
  using Base::swap;
  using Base::is_virtual;
#endif

public:
  /** @name Creation */ //@{
  Periodic_3_Delaunay_triangulation_3(
      const Iso_cuboid& domain = Iso_cuboid(0,0,0,1,1,1),
      const Geometric_traits& gt = Geometric_traits())
    : Base(domain, gt) {}

  // copy constructor duplicates vertices and cells
  Periodic_3_Delaunay_triangulation_3(
      const Periodic_3_Delaunay_triangulation_3& tr) : Base(tr) { 
    CGAL_triangulation_expensive_postcondition( is_valid() );  
  }

  template < typename InputIterator >
  Periodic_3_Delaunay_triangulation_3(InputIterator first, InputIterator last,
      const Iso_cuboid& domain = Iso_cuboid(0,0,0,1,1,1),
      const Geometric_traits& gt = Geometric_traits() )
    : Base(domain, gt) {
    insert(first, last);
  }
  //@}

  /** @name Insertion */ //@{
  Vertex_handle insert(const Point & p, Cell_handle start = Cell_handle()) {
    Conflict_tester tester(p, this);
    Point_hider hider;
    return Base::insert_in_conflict(p, start, tester, hider);
  }

  Vertex_handle insert(const Point & p, Locate_type lt, Cell_handle c,
      int li, int lj) {
    Conflict_tester tester(p, this);
    Point_hider hider;
    return Base::insert_in_conflict(p,lt,c,li,lj, tester,hider);
  }

  template < class InputIterator >
  int insert(InputIterator first, InputIterator last,
      bool is_large_point_set = false) {
    int n = number_of_vertices();
    // The heuristic discards the existing triangulation so it can only be
    // applied to empty triangulations.
    if (n!=0) is_large_point_set = false;

    std::vector<Point> points(first, last);
    std::random_shuffle (points.begin(), points.end());
    spatial_sort (points.begin(), points.end(), geom_traits());

    Cell_handle hint;
    std::vector<Vertex_handle> dummy_points, double_vertices;
    if (is_large_point_set)
      dummy_points = insert_dummy_points();

    Conflict_tester tester(*points.begin(),this);
    Point_hider hider;
    double_vertices = Base::insert_in_conflict(
	points.begin(),points.end(),hint,tester,hider);
    
    if (is_large_point_set) {
      typedef CGAL::Periodic_3_triangulation_remove_traits_3< Gt > P3removeT;
      typedef CGAL::Delaunay_triangulation_3< P3removeT > DT;
      typedef Vertex_remover< DT > Remover;
      P3removeT remove_traits(domain());
      DT dt(remove_traits);
      Remover remover(this,dt);
      Conflict_tester t(this);
      for (unsigned int i=0; i<dummy_points.size(); i++) {
	if (std::find(double_vertices.begin(), double_vertices.end(),
		dummy_points[i]) == double_vertices.end())
	  Base::remove(dummy_points[i],remover,t);
      }
    }

    return number_of_vertices() - n;
  }
  //@}

  /** @name Point moving */ //@{
  Vertex_handle move_point(Vertex_handle v, const Point & p);
  //@}

public:
  /** @name Removal */ //@{
  void remove(Vertex_handle v);

  template < typename InputIterator >
  int remove(InputIterator first, InputIterator beyond) {
    int n = number_of_vertices();
    while (first != beyond) {
      remove(*first);
      ++first;
    }
    return n - number_of_vertices();
  }
  //@}

public:
  /** @name Wrapping the traits */ //@{
  Oriented_side side_of_oriented_sphere(const Point &p, const Point &q,
      const Point &r, const Point &s, const Point &t) const {
    return geom_traits().side_of_oriented_sphere_3_object()(p,q,r,s,t);
  }
  Oriented_side side_of_oriented_sphere(const Point &p, const Point &q,
      const Point &r, const Point &s, const Point &t, const Offset &o_p,
      const Offset &o_q, const Offset &o_r, const Offset &o_s,
      const Offset &o_t) const {
    return geom_traits().side_of_oriented_sphere_3_object()(
	p,q,r,s,t,o_p,o_q,o_r,o_s,o_t);
  }
  Comparison_result compare_distance(const Point &p, const Point &q,
      const Point &r) const {
      return geom_traits().compare_distance_3_object()(p, q, r);
  }
  Comparison_result compare_distance(const Point &p, const Point &q,
      const Point &r, const Offset &o_p, const Offset &o_q,
      const Offset &o_r) const {
    return geom_traits().compare_distance_3_object()(p, q, r, o_p, o_q, o_r);
  }
  //@}

public:
  /** @name Queries */ //@{
  Bounded_side side_of_sphere1(const Cell_handle& c, const Point& p,
      const Offset & offset = Offset(), bool perturb = false) const;

  Bounded_side side_of_sphere(const Cell_handle& c, const Point& p,
      const Offset & offset = Offset(), bool perturb = false) const{
    Bounded_side bs = ON_UNBOUNDED_SIDE;
    int i=0;
    // TODO: optimize which copies to check depending on the offsets in
    // the cell.
    while (bs == ON_UNBOUNDED_SIDE && i<8) {
      bs= side_of_sphere1(c,p,combine_offsets(offset,int_to_off(i)),perturb);
      i++;
    }
    return bs;
  }

  Vertex_handle nearest_vertex(const Point& p,
      Cell_handle c = Cell_handle()) const;
  Vertex_handle nearest_vertex_in_cell(const Cell_handle& c,
      const Point & p, const Offset & offset = Offset()) const;

  /// Undocumented wrapper for find_conflicts.
  template <class OutputIteratorBoundaryFacets, class OutputIteratorCells>
  std::pair<OutputIteratorBoundaryFacets, OutputIteratorCells>
  find_conflicts(const Point &p, Cell_handle c,
      OutputIteratorBoundaryFacets bfit, OutputIteratorCells cit) const {
    Triple<OutputIteratorBoundaryFacets,OutputIteratorCells,Emptyset_iterator>
    t = find_conflicts(p, c, bfit, cit, Emptyset_iterator());
    return std::make_pair(t.first, t.second);
  }

  template <class OutputIteratorBoundaryFacets, class OutputIteratorCells,
            class OutputIteratorInternalFacets>
  Triple<OutputIteratorBoundaryFacets, OutputIteratorCells,
         OutputIteratorInternalFacets>
  find_conflicts(const Point &p, Cell_handle c,
      OutputIteratorBoundaryFacets bfit, OutputIteratorCells cit,
      OutputIteratorInternalFacets ifit) const;
  
  /// Returns the vertices on the boundary of the conflict hole.
  template <class OutputIterator>
  OutputIterator vertices_in_conflict(const Point&p, Cell_handle c,
      OutputIterator res) const;

  bool is_Gabriel(Cell_handle c, int i) const;
  bool is_Gabriel(Cell_handle c, int i, int j) const;
  bool is_Gabriel(const Facet& f)const {
    return is_Gabriel(f.first, f.second);
  }
  bool is_Gabriel(const Edge& e) const {
    return is_Gabriel(e.first, e.second, e.third);
  }
  //@}
  
private:
  /** @name Voronoi diagram helpers */ //@{
  bool is_canonical(const Facet &f) const {
    if (number_of_sheets() == make_array(1,1,1)) return true;
    Offset cell_off0 = Offset(f.first->offset((f.second+1)&3));
    Offset cell_off1 = Offset(f.first->offset((f.second+2)&3));
    Offset cell_off2 = Offset(f.first->offset((f.second+3)&3));
    Offset diff_off((cell_off0.x() == 1 
		     && cell_off1.x() == 1 
		     && cell_off2.x() == 1)?-1:0,
		    (cell_off0.y() == 1 
		     && cell_off1.y() == 1
		     && cell_off2.y() == 1)?-1:0,
		    (cell_off0.z() == 1 
		     && cell_off1.z() == 1
		     && cell_off2.z() == 1)?-1:0);
    Offset off0 = combine_offsets(get_offset(f.first, (f.second+1)&3),
			   diff_off);
    Offset off1 = combine_offsets(get_offset(f.first, (f.second+2)&3),
			   diff_off);
    Offset off2 = combine_offsets(get_offset(f.first, (f.second+3)&3),
			   diff_off);
    
    // If there is one offset with entries larger than 1 then we are
    // talking about a vertex that is too far away from the original
    // domain to belong to a canonical triangle.
    if (off0.x() > 1) return false;
    if (off0.y() > 1) return false;
    if (off0.z() > 1) return false;
    if (off1.x() > 1) return false;
    if (off1.y() > 1) return false;
    if (off1.z() > 1) return false;
    if (off2.x() > 1) return false;
    if (off2.y() > 1) return false;
    if (off2.z() > 1) return false;

    // If there is one direction of space for which all offsets are
    // non-zero then the edge is not canonical because we can
    // take the copy closer towards the origin in that direction.
    int offx = off0.x() & off1.x() & off2.x();
    int offy = off0.y() & off1.y() & off2.y();
    int offz = off0.z() & off1.z() & off2.z();

    return (offx == 0 && offy == 0 && offz == 0);
  }
  //@}

public:
  /** @name Voronoi diagram */ //@{
  Point dual(Cell_handle c) const {
    CGAL_triangulation_precondition(c != Cell_handle());
    Point v = geom_traits().construct_circumcenter_3_object()(
        c->vertex(0)->point(), c->vertex(1)->point(),
	c->vertex(2)->point(), c->vertex(3)->point(),
        get_offset(c,0), get_offset(c,1),
	get_offset(c,2), get_offset(c,3));
    // TODO: check that v lies within the domain. If not: translate
    return v;
  }
  Periodic_segment dual(const Facet & f) const {
    return dual( f.first, f.second );
  }
  Periodic_segment dual(Cell_handle c, int i) const{
    CGAL_triangulation_precondition(c != Cell_handle());
    Offset o = get_neighbor_offset(c,i,c->neighbor(i));
    return make_array(std::make_pair(dual(c),Offset()),
		      std::make_pair(dual(c->neighbor(i)),o));
  }
  template <class Stream>
  Stream& draw_dual(Stream& os) {
    CGAL_assertion_code( unsigned int i = 0; )
    for (Facet_iterator fit = facets_begin(), end = facets_end();
	 fit != end; ++fit) {
      if (!is_canonical(*fit)) continue;
      CGAL_assertion_code ( ++i; )
      Segment pso = segment(dual(*fit));
      os << pso;
    }
    CGAL_assertion( i == number_of_facets() );
    return os;
  }
  //@}
  
  /** @name Checking */ //@{
  bool is_valid(bool verbose = false, int level = 0) const;
  bool is_valid(Cell_handle c, bool verbose = false, int level = 0) const;
  //@}

private:
  class Conflict_tester;
  template <class DT> struct Vertex_remover;
  class Point_hider;
};

template < class GT, class Tds >
typename Periodic_3_Delaunay_triangulation_3<GT,Tds>::Vertex_handle
Periodic_3_Delaunay_triangulation_3<GT,Tds>::nearest_vertex(const Point& p,
    Cell_handle start) const {
  if (number_of_vertices() == 0)
    return Vertex_handle();

  Locate_type lt;
  int li, lj;
  Cell_handle c = locate(p, lt, li, lj, start);
  Offset o = get_location_offset(p,Offset(),c);
  if (lt == Base::VERTEX) return c->vertex(li);

  // - start with the closest vertex from the located cell.
  // - repeatedly take the nearest of its incident vertices if any
  // - if not, we're done.
  Vertex_handle nearest = nearest_vertex_in_cell(c, p, o);
  std::vector<Vertex_handle> vs;
  vs.reserve(32);
  while (true) {
    Vertex_handle tmp = nearest;
    incident_vertices(nearest, std::back_inserter(vs));
    for (typename std::vector<Vertex_handle>::const_iterator
       vsit = vs.begin(); vsit != vs.end(); ++vsit)
      tmp = (compare_distance(p,tmp->point(),(*vsit)->point(),
			      o,get_offset(tmp),get_offset(*vsit))
	     == SMALLER) ? tmp : *vsit;
    if (tmp == nearest)
      break;
    vs.clear();
    nearest = tmp;
  }

  return nearest;
}

/// Returns the finite vertex of the cell c which is the closest to p.
template < class GT, class Tds >
typename Periodic_3_Delaunay_triangulation_3<GT,Tds>::Vertex_handle
Periodic_3_Delaunay_triangulation_3<GT,Tds>::nearest_vertex_in_cell(
    const Cell_handle& c, const Point & p, const Offset & o) const {
  CGAL_triangulation_precondition(number_of_vertices() != 0);
  Vertex_handle nearest = c->vertex(0);
  for (int i=1 ; i<4 ; i++)
    nearest = (compare_distance(p,nearest->point(),c->vertex(i)->point(),
        o,get_offset(nearest),get_offset(c->vertex(i))) == SMALLER) ?
      nearest : c->vertex(i);
  return nearest;
}

// ############################################################################


template < class Gt, class Tds >
typename Periodic_3_Delaunay_triangulation_3<Gt,Tds>::Vertex_handle
Periodic_3_Delaunay_triangulation_3<Gt,Tds>::
move_point(Vertex_handle v, const Point & p) {
  //CGAL_triangulation_precondition(! is_infinite(v));
  CGAL_triangulation_expensive_precondition(is_vertex(v));
  // Remember an incident vertex to restart
  // the point location after the removal.
  Cell_handle c = v->cell();
  //Vertex_handle old_neighbor = c->vertex(c->index(v) == 0 ? 1 : 0);
  //  CGAL_triangulation_assertion(old_neighbor != v);

  remove(v);

  if (number_of_vertices() == 0)
    return insert(p);
  return insert(p);//, old_neighbor->cell());
}

template < class Gt, class Tds >
void Periodic_3_Delaunay_triangulation_3<Gt,Tds>::remove(Vertex_handle v)
{
  if ( !is_vertex(v) ) return;
  typedef CGAL::Periodic_3_triangulation_remove_traits_3< Gt > P3removeT;
  typedef CGAL::Delaunay_triangulation_3< P3removeT >
    Euclidean_triangulation;
  typedef Vertex_remover< Euclidean_triangulation > Remover;
  P3removeT remove_traits(domain());
  Euclidean_triangulation tmp(remove_traits);
  Remover remover(this, tmp);
  Conflict_tester ct(this);

  Base::remove(v, remover, ct);
  CGAL_expensive_assertion(is_valid());
}

template < class Gt, class Tds >
template <class OutputIteratorBoundaryFacets, class OutputIteratorCells,
          class OutputIteratorInternalFacets>
Triple<OutputIteratorBoundaryFacets, OutputIteratorCells,
       OutputIteratorInternalFacets>
Periodic_3_Delaunay_triangulation_3<Gt,Tds>::find_conflicts( const Point
&p,
    Cell_handle c, OutputIteratorBoundaryFacets bfit,
    OutputIteratorCells cit, OutputIteratorInternalFacets ifit) const {
  CGAL_triangulation_precondition(number_of_vertices() != 0);

  std::vector<Facet> facets;
  facets.reserve(64);
  std::vector<Cell_handle> cells;
  cells.reserve(32);

  Conflict_tester tester(p, this);
  Triple<typename std::back_insert_iterator<std::vector<Facet> >,
         typename std::back_insert_iterator<std::vector<Cell_handle> >,
         OutputIteratorInternalFacets> tit = Base::find_conflicts(c, tester,
              make_triple(std::back_inserter(facets),
                      std::back_inserter(cells), ifit));
  ifit = tit.third;

  // Reset the conflict flag on the boundary.
  for(typename std::vector<Facet>::iterator fit=facets.begin();
  fit != facets.end(); ++fit) {
    fit->first->neighbor(fit->second)->set_in_conflict_flag(0);
    *bfit++ = *fit;
  }

  // Reset the conflict flag in the conflict cells.
  for(typename std::vector<Cell_handle>::iterator ccit=cells.begin();
      ccit != cells.end(); ++ccit) {
    (*ccit)->set_in_conflict_flag(0);
    *cit++ = *ccit;
  }

  for (typename std::vector<Vertex_handle>::iterator
	 voit = this->v_offsets.begin();
       voit != this->v_offsets.end() ; ++voit) {
    (*voit)->clear_offset();
  }
  this->v_offsets.clear();

  return make_triple(bfit, cit, ifit);
}

template < class Gt, class Tds >
template <class OutputIterator>
OutputIterator
Periodic_3_Delaunay_triangulation_3<Gt,Tds>::vertices_in_conflict(
    const Point&p, Cell_handle c, OutputIterator res) const {
  if (number_of_vertices() == 0) return res;

  // Get the facets on the boundary of the hole.
  std::vector<Facet> facets;
  find_conflicts(p, c, std::back_inserter(facets), Emptyset_iterator());
  
  // Then extract uniquely the vertices.
  std::set<Vertex_handle> vertices;
  for (typename std::vector<Facet>::const_iterator i = facets.begin();
       i != facets.end(); ++i) {
    vertices.insert(i->first->vertex((i->second+1)&3));
    vertices.insert(i->first->vertex((i->second+2)&3));
    vertices.insert(i->first->vertex((i->second+3)&3));
  }
  
  // TODO: check whether the flags in the offsets need to be reset
  // using v_offsets
  return std::copy(vertices.begin(), vertices.end(), res);
}

template < class Gt, class Tds >
Bounded_side Periodic_3_Delaunay_triangulation_3<Gt,Tds>::
side_of_sphere1(const Cell_handle &c, const Point &q,
    const Offset &offset, bool perturb ) const {
//TODO: check what this code was supposed to do.
#if 0
  Point *pts[4]; 
  if (t->number_of_sheets() == make_array(1,1,1)) {
    int off;
    for (int i=0; i<4; i++) {
      off = c->offset(i);
      pts[i] = &(c->vertex(i)->point(off));
      if (!(c->vertex(i)->get_flag(off))) {
        //TODO: this addition is not exact!
        c->vertex(i)->set_point(c->vertex(i)->point()+off, off);
        pts[i] = &(c->vertex(i)->point(off));
      }
    }

    return (Bounded_side) Side_of_oriented_sphere_with_perturbation_3()(
          *pts[0], *pts[1], *pts[2], *pts[3], pt,
          Offset(), Offset(), Offset(), Offset(), offset, perturb);
  }
#endif

  Point p0 = c->vertex(0)->point(),
        p1 = c->vertex(1)->point(),
        p2 = c->vertex(2)->point(),
        p3 = c->vertex(3)->point();
  Offset o0 = get_offset(c,0),
        o1 = get_offset(c,1),
        o2 = get_offset(c,2),
        o3 = get_offset(c,3),
        oq = offset;

  Oriented_side os = ON_NEGATIVE_SIDE;
  os= side_of_oriented_sphere(p0, p1, p2, p3, q, o0, o1, o2, o3, oq);

  if (os != ON_ORIENTED_BOUNDARY || !perturb)
    return (Bounded_side) os;

  //We are now in a degenerate case => we do a symbolic perturbation. 
  // We sort the points lexicographically.
  Periodic_point pts[5] = {std::make_pair(p0,o0), std::make_pair(p1,o1),
			   std::make_pair(p2,o2), std::make_pair(p3,o3),
			   std::make_pair(q,oq)};
  const Periodic_point *points[5] ={&pts[0],&pts[1],&pts[2],&pts[3],&pts[4]};

  std::sort(points, points+5,
      typename Base::template Perturbation_order<
	  typename Gt::Compare_xyz_3 >(
	  geom_traits().compare_xyz_3_object() ) );
  // We successively look whether the leading monomial, then 2nd monomial
  // of the determinant has non null coefficient.
  // 2 iterations are enough (cf paper)
  for (int i=4; i>2; --i) {
    if (points[i] == &pts[4]) {
      CGAL_assertion(orientation(p0, p1, p2, p3, o0, o1, o2, o3)
          == POSITIVE);
      // since p0 p1 p2 p3 are non coplanar and positively oriented
      return ON_UNBOUNDED_SIDE;
    }
    Orientation o;
    if (points[i] == &pts[3] && 
        (o = orientation(p0, p1, p2, q, o0, o1, o2, oq)) != COPLANAR ) {
      return (Bounded_side) o;
    }
    if (points[i] == &pts[2] && 
        (o = orientation(p0, p1, q, p3, o0, o1, oq, o3)) != COPLANAR ) {
      return (Bounded_side) o;
    }
    if (points[i] == &pts[1] && 
        (o = orientation(p0, q, p2, p3, o0, oq, o2, o3)) != COPLANAR ) {
      return (Bounded_side) o;
    }
    if (points[i] == &pts[0] && 
        (o = orientation(q, p1, p2 ,p3, oq, o1, o2, o3)) != COPLANAR ) {
      return (Bounded_side) o;
    }
  }

  CGAL_triangulation_assertion(false);
  return ON_UNBOUNDED_SIDE;
}

template < class Gt, class Tds >
bool Periodic_3_Delaunay_triangulation_3<Gt,Tds>::
is_Gabriel(Cell_handle c, int i) const {
  CGAL_triangulation_precondition(number_of_vertices() != 0);

  typename Geom_traits::Side_of_bounded_sphere_3
    side_of_bounded_sphere =
    geom_traits().side_of_bounded_sphere_3_object();

  if (side_of_bounded_sphere (
          c->vertex(vertex_triple_index(i,0))->point(),
	  c->vertex(vertex_triple_index(i,1))->point(),
	  c->vertex(vertex_triple_index(i,2))->point(),
          c->vertex(i)->point(),
          get_offset(c,vertex_triple_index(i,0)),
          get_offset(c,vertex_triple_index(i,1)),
          get_offset(c,vertex_triple_index(i,2)),
          get_offset(c,i) ) == ON_BOUNDED_SIDE ) return false;
  Cell_handle neighbor = c->neighbor(i);
  int in = neighbor->index(c);

  if (side_of_bounded_sphere(
          c->vertex(vertex_triple_index(i,0))->point(),
	  c->vertex(vertex_triple_index(i,1))->point(),
	  c->vertex(vertex_triple_index(i,2))->point(),
          neighbor->vertex(in)->point(),
          get_offset(c,vertex_triple_index(i,0)),
          get_offset(c,vertex_triple_index(i,1)),
          get_offset(c,vertex_triple_index(i,2)),
          get_offset(neighbor, in) ) == ON_BOUNDED_SIDE )
    return false;
  
  return true;
}

template < class Gt, class Tds >
bool Periodic_3_Delaunay_triangulation_3<Gt,Tds>::
is_Gabriel(Cell_handle c, int i, int j) const {
  typename Geom_traits::Side_of_bounded_sphere_3
    side_of_bounded_sphere =
    geom_traits().side_of_bounded_sphere_3_object();

  Facet_circulator fcirc = incident_facets(c,i,j),
    fdone(fcirc);
  Vertex_handle v1 = c->vertex(i);
  Offset off1 = Offset(c->offset(i));
  Vertex_handle v2 = c->vertex(j);
  Offset off2 = Offset(c->offset(j));
  do {
    // test whether the vertex of cc opposite to *fcirc
    // is inside the sphere defined by the edge e = (s, i,j)
    Cell_handle cc = (*fcirc).first;
    int ii = (*fcirc).second;
    Offset off3 = Offset(cc->offset(ii));
    if (side_of_bounded_sphere( v1->point(), v2->point(), cc->vertex(ii)->point(), off1, off2, off3)  
	== ON_BOUNDED_SIDE ) return false;
  } while(++fcirc != fdone);
  return true;
}

template < class Gt, class Tds >
bool
Periodic_3_Delaunay_triangulation_3<Gt,Tds>::
is_valid(bool verbose, int level) const
{ 
  if (!Base::is_valid(verbose, level)) {
    if (verbose)
      std::cerr << "Delaunay: invalid base" << std::endl;
    return false;
  }

  Conflict_tester tester(this);
  if (!is_valid_conflict(tester, verbose, level)) {
    if (verbose)
      std::cerr << "Delaunay: conflict problems" << std::endl;
    return false;
  }

  if (verbose)
    std::cerr << "Delaunay valid triangulation" << std::endl;
  return true;
}

template < class GT, class TDS >
bool
Periodic_3_Delaunay_triangulation_3<GT,TDS>::
is_valid(Cell_handle ch, bool verbose, int level) const {
  bool error = false;
  if (!Base::is_valid(ch, verbose, level)) {
    error = true;
    if (verbose) {
      std::cerr << "geometrically invalid cell" << std::endl;
      for (int i=0; i<4; i++ )
	std::cerr << ch->vertex(i)->point() << ", ";
      std::cerr << std::endl;
    }
  }
  for (Vertex_iterator vit = vertices_begin(); vit != vertices_end(); ++ vit) {
    for (int i=-1; i<=1; i++)
      for (int j=-1; j<=1; j++)
	for (int k=-1; k<=1; k++) {
	  if (side_of_sphere1(ch, vit->point(), Offset(i,j,k))
	      != ON_UNBOUNDED_SIDE) {
	    error = true;
	    if (verbose) {
	      std::cerr << "Delaunay invalid cell" << std::endl;
	      for (int i=0; i<4; i++ )
		std::cerr << ch->vertex(i)->point() << ", ";
	      std::cerr << std::endl;
	    }
	  }
	}
  }
  return !error;
}

template < class GT, class Tds >
class Periodic_3_Delaunay_triangulation_3<GT,Tds>::Conflict_tester
{
  // stores a pointer to the triangulation,
  // a point, and an offset
  const Self *t;
  Point p;
  mutable Offset o;

public:
  /// Constructor
  Conflict_tester(const Self *_t) : t(_t), p(Point()) {}
  Conflict_tester(const Point &pt, const Self *_t) : t(_t), p(pt) { }
  
  /** The functor
    *
    * gives true if the circumcircle of c contains p
    */
  bool operator()(const Cell_handle c, const Offset &off) const {
    return (t->side_of_sphere1(c, p, t->combine_offsets(o, off), true)
        == ON_BOUNDED_SIDE);
  }

  bool operator()(const Cell_handle c, const Point& pt,
      const Offset &off) const {
    return (t->side_of_sphere1(c, pt, o + off, true) == ON_BOUNDED_SIDE);
  }
  
  int compare_weight(Point, Point) const
  {
    return 0;
  }
  
  bool test_initial_cell(Cell_handle c, const Offset &off) const
  {
    if (!(operator()(c, off)))
      CGAL_assertion(false);
    return true;
  }
  
  void set_point(const Point &_p) {
    p = _p;
  }

  void set_offset(const Offset &off) const {
    o = off;
  }
  
  const Offset &get_offset() const {
    return o;
  }
  
  const Point &point() const {
    return p;
  }
  
};

template <class GT, class Tds>
template <class EuclideanTriangulation>
struct Periodic_3_Delaunay_triangulation_3<GT,Tds>::Vertex_remover
{
  typedef typename Periodic_3_Delaunay_triangulation_3<GT,Tds>::Conflict_tester
      Conflict_tester;
  typedef typename Periodic_3_Delaunay_triangulation_3<GT,Tds>::Point_hider
      Point_hider;

  typedef typename std::vector<Point>::iterator Hidden_points_iterator;

  // TODO: All these typedefs are only needed in the remove.
  // If they are not different for the Regular_conflict_tester, then
  // they should be moved to the remove.
  
  typedef EuclideanTriangulation Triangulation;

  typedef Triple < Vertex_handle, Vertex_handle, Vertex_handle > Vertex_triple;
  
  typedef typename Triangulation::Triangulation_data_structure TDSE;
  typedef typename Triangulation::Cell_handle        CellE_handle;
  typedef typename Triangulation::Vertex_handle      VertexE_handle;
  typedef typename Triangulation::Facet              FacetE;
  typedef typename Triangulation::Finite_cells_iterator Finite_cellsE_iterator;
  
  typedef Triple< VertexE_handle, VertexE_handle, VertexE_handle >
      VertexE_triple;
  
  typedef std::map<Vertex_triple,Facet> Vertex_triple_Facet_map;
  typedef std::map<Vertex_triple, FacetE> Vertex_triple_FacetE_map;
  typedef typename Vertex_triple_FacetE_map::iterator
      Vertex_triple_FacetE_map_it;
  
  Vertex_remover(const Self *t, Triangulation &tmp_) : _t(t),tmp(tmp_) {}
  
  const Self *_t;
  Triangulation &tmp;
  
  void add_hidden_points(Cell_handle) {
    std::copy (hidden_points_begin(), hidden_points_end(), 
        std::back_inserter(hidden));
  }

  Hidden_points_iterator hidden_points_begin() {
    return hidden.begin();
  }
  Hidden_points_iterator hidden_points_end() {
    return hidden.end();
  }
  //private:
  // The removal of v may un-hide some points,
  // Space functions output them.
  std::vector<Point> hidden;
};

template < class GT, class Tds>
class Periodic_3_Delaunay_triangulation_3<GT,Tds>::Point_hider
{
public:
  Point_hider() {}

  template <class InputIterator>
  inline void set_vertices(InputIterator, InputIterator) const {}
  inline void reinsert_vertices(Vertex_handle ) {}
  inline Vertex_handle replace_vertex(Cell_handle c, int index,
                                      const Point &) {
    return c->vertex(index);
  }
  inline void hide_point(Cell_handle, const Point &) {}

  inline void hide(Point &, Cell_handle ) const {
    CGAL_assertion(false);
  }

  inline void do_hide(const Point &, Cell_handle ) const {
    CGAL_assertion(false);
  }
  template < class Tester > 
  inline bool replace_vertex(const Point &, Vertex_handle ,
      const Tester &) const {
    return true;
  }
  template <class Conflict_tester>
  inline void hide_points(Vertex_handle,
      const Conflict_tester &) {
    // No points to hide in the Delaunay triangulation.
  }

};

CGAL_END_NAMESPACE

#endif // CGAL_PERIODIC_3_DELAUNAY_TRIANGULATION_3_H