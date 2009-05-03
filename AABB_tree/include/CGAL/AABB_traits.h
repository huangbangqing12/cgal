// Copyright (c) 2009 INRIA Sophia-Antipolis (France).
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
// $URL: $
// $Id: $
//
//
// Author(s) : Stéphane Tayeb, Pierre Alliez
//
//******************************************************************************
// File Description :
//
//******************************************************************************

#ifndef AABB_TRAITS_H_
#define AABB_TRAITS_H_

#include <CGAL/Bbox_3.h>
#include <CGAL/AABB_intersections.h>

namespace CGAL {

/**
 * @class AABB_traits
 *
 *
 */
template<typename GeomTraits, typename AABB_primitive>
class AABB_traits
{
public:
  /// AABBTraits concept types
  typedef typename CGAL::Bbox_3 Bounding_box;

  typedef AABB_primitive Primitive;
  typedef typename AABB_primitive::Datum Datum;
  typedef typename GeomTraits::Sphere_3 Sphere;

  // TOFIX: Workaround for weighted_point
#ifndef AABB_KERNEL_USE_WEIGHTED_POINT
  typedef typename GeomTraits::Point_3 Point;
#else
  typedef typename GeomTraits::Point_3::Point Point;
#endif

  typedef typename std::pair<typename Point, typename Primitive> Point_and_primitive;

  // types for search tree
  // TOFIX: how can we avoid repeating those?
  typedef typename GeomTraits::FT FT;
  typedef typename GeomTraits::Point_3 Point_3;
  typedef typename GeomTraits::Sphere_3 Sphere_3;
  typedef typename GeomTraits::Iso_cuboid_3 Iso_cuboid_3;
  typedef typename GeomTraits::Construct_center_3 Construct_center_3;
  typedef typename GeomTraits::Construct_iso_cuboid_3 Construct_iso_cuboid_3;
  typedef typename GeomTraits::Construct_min_vertex_3 Construct_min_vertex_3;
  typedef typename GeomTraits::Construct_max_vertex_3 Construct_max_vertex_3;
  typedef typename GeomTraits::Compute_squared_radius_3 Compute_squared_radius_3;
  typedef typename GeomTraits::Cartesian_const_iterator_3 Cartesian_const_iterator_3;
  typedef typename GeomTraits::Construct_cartesian_const_iterator_3
                     Construct_cartesian_const_iterator_3;

  /// Constructor
  AABB_traits() { };

  /// Non-virtual Destructor
  ~AABB_traits() { };

  /// Comparison functions
  static bool less_x(const Primitive& pr1, const Primitive& pr2)
  { return pr1.reference_point().x() < pr2.reference_point().x(); }
  static bool less_y(const Primitive& pr1, const Primitive& pr2)
  { return pr1.reference_point().y() < pr2.reference_point().y(); }
  static bool less_z(const Primitive& pr1, const Primitive& pr2)
  { return pr1.reference_point().z() < pr2.reference_point().z(); }

  /// UNDOCUMENTED FEATURE
  /// TODO: see what to do
  /**
   * @brief Sorts [first,beyond[
   * @param first iterator on first element
   * @param beyond iterator on beyond element
   * @param bbox the bounding box of [first,beyond[
   *
   * Sorts the range defined by [first,beyond[. Sort is achieved on bbox longuest
   * axis, using the comparison function <dim>_less_than (dim in {x,y,z})
   */
  template<typename PrimitiveIterator>
  void sort_primitives(PrimitiveIterator first,
                       PrimitiveIterator beyond,
                       const Bounding_box& bbox) const;

  /**
   * Computes the bounding box of a set of primitives
   * @param first an iterator on the first primitive
   * @param beyond an iterator on the past-the-end primitive
   * @return the bounding box of the primitives of the iterator range
   */
  template<typename ConstPrimitiveIterator>
  Bounding_box compute_bbox(ConstPrimitiveIterator first,
                            ConstPrimitiveIterator beyond) const;

  template<typename Query>
  bool do_intersect(const Query& q, const Bounding_box& bbox) const
  {
    // AABB tree package call TODO: extend kernel
    return CGAL::do_intersect(q, bbox);
  }

  template<typename Query>
  bool do_intersect(const Query& q, const Primitive& pr) const
  {
    return GeomTraits().do_intersect_3_object()(q, pr.datum());
  }

  template<typename Query>
  bool intersection(const Query& q,
                    const Primitive& pr,
                    Point_and_primitive& result) const;

  Sphere sphere(const Point& center,
                const Point& hint) const
  {
    return GeomTraits().construct_sphere_3_object()
      (center, GeomTraits().compute_squared_distance_3_object()(center, hint));
  }

  template <typename Query>
  Point nearest_point(const Query& q,
                           const Primitive& pr,
                           const Point& bound) const
  {
    return CGAL::nearest_point_3(q, pr.datum(), bound);
  }

private:
  /**
   * @brief Computes bounding box of one primitive
   * @param pr the primitive
   * @return the bounding box of the primitive \c pr
   */
  Bounding_box compute_bbox(const Primitive& pr) const
  {
    return pr.datum().bbox();
  }

  typedef enum { CGAL_AXIS_X = 0,
                 CGAL_AXIS_Y = 1,
                 CGAL_AXIS_Z = 2} Axis;

  Axis longest_axis(const Bounding_box& bbox) const;

private:
  // Disabled copy constructor & assignment operator
  typedef AABB_traits<GeomTraits, Primitive> Self;
  AABB_traits(const Self& src);
  Self& operator=(const Self& src);

};  // end class AABB_traits


template<typename GT, typename P>
template<typename PrimitiveIterator>
void
AABB_traits<GT,P>::sort_primitives(PrimitiveIterator first,
                                   PrimitiveIterator beyond,
                                   const Bounding_box& bbox) const
{
  PrimitiveIterator middle = first + (beyond - first)/2;
  switch(longest_axis(bbox))
  {
  case CGAL_AXIS_X: // sort along x
    std::nth_element(first, middle, beyond, less_x);
    break;
  case CGAL_AXIS_Y: // sort along y
    std::nth_element(first, middle, beyond, less_y);
    break;
  case CGAL_AXIS_Z: // sort along z
    std::nth_element(first, middle, beyond, less_z);
    break;
  default:
    CGAL_error();
  }
}

template<typename GT, typename P>
template<typename ConstPrimitiveIterator>
typename AABB_traits<GT,P>::Bounding_box
AABB_traits<GT,P>::compute_bbox(ConstPrimitiveIterator first,
                                 ConstPrimitiveIterator beyond) const
{
  Bounding_box bbox = compute_bbox(*first);
  for(++first; first != beyond; ++first)
  {
    bbox = bbox + compute_bbox(*first);
  }
  return bbox;
}


template<typename GT, typename P>
template<typename Query>
bool
AABB_traits<GT,P>::intersection(const Query& q,
                                const P& pr,
                                Point_and_primitive& result) const
{
  // TODO: implement a real intersection construction method
  // do_intersect is needed here because we construct intersection between
  // pr.datum().supporting_plane() and q
  if ( ! do_intersect(q,pr) )
  {
    return false;
  }

  // AABB tree package call
  Datum datum = pr.datum();
  CGAL::Object intersection_obj = CGAL::intersection(datum, q);
  Point point;
  if(CGAL::assign(point, intersection_obj))
     result = std::pair<point,pr>;
  else
     return false;
}

//-------------------------------------------------------
// Private methods
//-------------------------------------------------------
template<typename GT, typename P>
typename AABB_traits<GT,P>::Axis
AABB_traits<GT,P>::longest_axis(const Bounding_box& bbox) const
{
  const double dx = bbox.xmax() - bbox.xmin();
  const double dy = bbox.ymax() - bbox.ymin();
  const double dz = bbox.zmax() - bbox.zmin();

  if(dx>=dy)
  {
    if(dx>=dz)
    {
      return CGAL_AXIS_X;
    }
    else // dz>dx and dx>=dy
    {
      return CGAL_AXIS_Z;
    }
  }
  else // dy>dx
  {
    if(dy>=dz)
    {
      return CGAL_AXIS_Y;
    }
    else  // dz>dy and dy>dx
    {
      return CGAL_AXIS_Z;
    }
  }
}


}  // end namespace CGAL

#endif // AABB_TRAITS_H_