// Copyright (c) 2007-09  ETH Zurich (France).
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
// Author(s)     : Gael Guennebaud, Laurent Saboret

#ifndef CGAL_APSS_RECONSTRUCTION_FUNCTION_H
#define CGAL_APSS_RECONSTRUCTION_FUNCTION_H

#include <vector>
#include <algorithm>

#include <CGAL/Point_with_normal_3.h>
#include <CGAL/make_surface_mesh.h>
#include <CGAL/Fast_orthogonal_k_neighbor_search.h>
#include <CGAL/Search_traits_3.h>
#include <CGAL/Surface_mesher/Implicit_surface_oracle_3.h>
#include <CGAL/Min_sphere_d.h>
#include <CGAL/Optimisation_d_traits_3.h>
#include <CGAL/surface_reconstruction_points_assertions.h>

CGAL_BEGIN_NAMESPACE


/// APSS_reconstruction_function computes an implicit function
/// that defines a Point Set Surface (PSS) based on
/// moving least squares (MLS) fitting of algebraic spheres.
///
/// See "Algebraic Point Set Surfaces" by Guennebaud and Gross [Guennebaud07].
///
/// Note that APSS reconstruction may create small "ghost" connected components
/// close to the reconstructed surface that you should delete.
/// For this purpose, you may call erase_small_polyhedron_connected_components()
/// after make_surface_mesh().
///
/// @heading Is Model for the Concepts:
/// Model of the 'ImplicitFunction' concept.
///
/// @heading Parameters:
/// @param Gt Geometric traits class.

template <class Gt>
class APSS_reconstruction_function
{
// Public types
public:

  typedef Gt Geom_traits; ///< Kernel's geometric traits

  typedef typename Geom_traits::FT FT;
  typedef typename Geom_traits::Point_3 Point; ///< == Point_3<Gt>
  typedef typename Geom_traits::Vector_3 Vector; ///< == Vector_3<Gt>
  typedef typename Geom_traits::Sphere_3 Sphere;

  typedef Point_with_normal_3<Gt> Point_with_normal; ///< == Point_with_normal_3<Gt>

// Private types
private:

  // Item in the Kd-tree: position (Point_3) + normal + index
  class KdTreeElement : public Point_with_normal
  {
  public:
    unsigned int index;

    KdTreeElement(const Origin& o = ORIGIN, unsigned int id=0)
      : Point_with_normal(o), index(id)
    {}
    template <class K>
    KdTreeElement(const Point_with_normal_3<K>& pwn, unsigned int id=0)
      : Point_with_normal(pwn), index(id)
    {}
    KdTreeElement(const Point& p, unsigned int id=0)
      : Point_with_normal(p), index(id)
    {}
    KdTreeElement(const KdTreeElement& other)
      : Point_with_normal(other), index(other.index)
    {}
  };

  // Helper class for the Kd-tree
  class KdTreeGT : public Geom_traits
  {
  public:
    typedef KdTreeElement Point_3;
  };

  class TreeTraits : public Search_traits_3<KdTreeGT>
  {
    public:
      typedef Point PointType;
  };

  typedef Fast_orthogonal_k_neighbor_search<TreeTraits> Neighbor_search;
  typedef typename Neighbor_search::Tree Tree;
  typedef typename Neighbor_search::Point_ptr_with_transformed_distance
                                    Point_ptr_with_transformed_distance;

// Public methods
public:

  /// Creates an APSS implicit function from a set of oriented points.
  ///
  /// @commentheading Precondition:
  /// InputIterator value_type must be convertible to Point_with_normal.
  ///
  /// @param first Iterator over first point.
  /// @param beyond Past-the-end iterator.
  /// @param k number of neighbors for APSS sphere fitting.
  template < class InputIterator >
  APSS_reconstruction_function(InputIterator first, InputIterator beyond,
                               unsigned int k)
  {
    // Allocate smart pointer to data
    m = new Private;
    m->cached_nearest_neighbor.first = 0;

    int nb_points = std::distance(first, beyond);

    // Number of nearest neighbors
    m->nofNeighbors = k;

    // Create kd-tree
    m->treeElements.reserve(nb_points);
    unsigned int i=0;
    for (InputIterator it=first ; it != beyond ; ++it,++i)
    {
      m->treeElements.push_back(KdTreeElement(*it,i));
    }
    m->tree = new Tree(m->treeElements.begin(), m->treeElements.end());

    // Compute the radius of each point = (distance max to k nearest neighbors)/2.
    // The union of these balls defines the surface definition domain.
    m->radii.resize(nb_points);
    // FIXME the radii should be computed by another component
    {
      int i=0;
      for (InputIterator it=first ; it != beyond ; ++it, ++i)
      {
        Neighbor_search search(*(m->tree), *it, 16);
        FT maxdist2 = search.begin()->second; // squared distance to furthest neighbor
        m->radii[i] = sqrt(maxdist2)/2.;
      }
    }

    // Compute bounding sphere
    Min_sphere_d< CGAL::Optimisation_d_traits_3<Gt> > ms3(first, beyond);
    m->bounding_sphere = Sphere(ms3.center(), ms3.squared_radius());

    // Find a point inside the surface.
    find_inner_point();

    // Dichotomy error when projecting point (squared)
    m->sqError = 1e-7 * Gt().compute_squared_radius_3_object()(m->bounding_sphere);
  }

  /// Copy constructor
  APSS_reconstruction_function(const APSS_reconstruction_function& other) {
    m = other.m;
    m->count++;
  }

  /// operator =()
  APSS_reconstruction_function& operator = (const APSS_reconstruction_function& other) {
    m = other.m;
    m->count++;
  }

  /// Destructor
  ~APSS_reconstruction_function() {
    if (--(m->count)==0)
      delete m;
  }

  /// Set number of neighbors for APSS sphere fitting.
  void set_numbers_of_neighbors(unsigned int k) {m->nofNeighbors = k;}

  /// Returns a sphere bounding the inferred surface.
  const Sphere& bounding_sphere() const
  {
    return m->bounding_sphere;
  }

private:

  /** Fit an algebraic sphere on a set of neigbors in a Moving Least Square sense.
  The weight function is scaled such that the weight of the furthest neighbor is 0.
  */
  void fit(const Neighbor_search& search) const
  {
    FT r2 = search.begin()->second; // squared distance to furthest neighbor

    Vector sumP(0,0,0);
    Vector sumN(0,0,0);
    FT sumDotPP = 0.;
    FT sumDotPN = 0.;
    FT sumW = 0.;

    r2 *= 1.001;
    FT invr2 = 1./r2;
    for (typename Neighbor_search::iterator it = search.begin(); it != search.end(); ++it)
    {
      Vector p = *(it->first) - CGAL::ORIGIN;
      const Vector& n = it->first->normal();
      FT w = 1. - it->second*invr2;
      w = w*w; w = w*w;

      sumP = add(sumP,mul(w,p));
      sumN = add(sumN,mul(w,n));
      sumDotPP += w * dot(p,p);
      sumDotPN += w * dot(p,n);
      sumW += w;
    }

    FT invSumW = 1./sumW;
    m->as.u4 = 0.5 * (sumDotPN - invSumW*dot(sumP,sumN))/(sumDotPP - invSumW*dot(sumP,sumP));
    m->as.u13 = mul(invSumW,add(sumN,mul(-2.*m->as.u4,sumP)));
    m->as.u0 = -invSumW*(dot(m->as.u13,sumP)+m->as.u4*sumDotPP);
    m->as.finalize();
  }

  /** Check whether the point 'p' is close to the input points or not.
      We assume that it's not if it is far from its nearest neighbor.
  */
  inline bool isValid(const Point_ptr_with_transformed_distance& nearest_neighbor, const Point& /* p */) const
  {
      FT r = 2. * m->radii[nearest_neighbor.first->index];
      return (r*r > nearest_neighbor.second);
  }

public:

  /// 'ImplicitFunction' interface: evaluate implicit function for any 3D point.
  //
  // Implementation note: this function is called a large number of times,
  // thus us heavily optimized. The bottleneck is Neighbor_search's constructor,
  // which we try to avoid calling.
  FT operator()(const Point& p) const
  {
    // Is 'p' close to the surface?
    // Optimization: test first if 'p' is close to one of the neighbors
    //               computed during the previous call.
    typename Geom_traits::Compute_squared_distance_3 sqd;
    if (m->cached_nearest_neighbor.first)
      m->cached_nearest_neighbor.second = sqd(p, *m->cached_nearest_neighbor.first);
    if (!(m->cached_nearest_neighbor.first && isValid(m->cached_nearest_neighbor, p)))
    {
      // Compute the nearest neighbor and cache it
      KdTreeElement query(p);
      Neighbor_search search_1nn(*(m->tree), query, 1);
      m->cached_nearest_neighbor = *(search_1nn.begin());

      // Is 'p' close to the surface?
      if (!isValid(m->cached_nearest_neighbor, p))
      {
        // If 'p' is far from the surface, project its nearest neighbor onto the surface...
        Vector n;
        Point pp = *m->cached_nearest_neighbor.first;
        project(pp,n,1);
        // ...and return the (signed) distance to the surface
        Vector h = sub(p,pp);
        return length(h) * ( dot(n,h)>0. ? 1. : -1.);
      }
    }

    // Compute k nearest neighbors and cache the nearest one
    KdTreeElement query(p);
    Neighbor_search search_knn(*(m->tree), query, m->nofNeighbors);
    m->cached_nearest_neighbor = search_nearest(search_knn);

    // If 'p' is close to the surface, fit an algebraic sphere
    // on a set of neigbors in a Moving Least Square sense.
    fit(search_knn);

    // return the distance to the sphere
    return m->as.euclideanDistance(p);
  }

  /// Returns a point located inside the inferred surface.
  Point get_inner_point() const
  {
    return m->inner_point;
  }

// Private methods:
private:

  const Point_ptr_with_transformed_distance& search_nearest(const Neighbor_search& search) const
  {
    typename Neighbor_search::iterator last=search.end(); --last;
    typename Neighbor_search::iterator nearest_it = last;
    for (typename Neighbor_search::iterator it = search.begin(); it != last; ++it)
      if (it->second < nearest_it->second)
        nearest_it = it;
    return *nearest_it;
  }

  /** Projects the point p onto the MLS surface.
  */
  void project(Point& p, unsigned int maxNofIterations = 20) const
  {
    Vector n;
    project(p,n,maxNofIterations);
  }

  /** Projects the point p onto the MLS surface, and returns an approximate normal.
  */
  void project(Point& p, Vector& n, unsigned int maxNofIterations = 20) const
  {
    Point source = p;

    FT delta2 = 0.;
    unsigned int countIter = 0;
    do {

      Neighbor_search search(*(m->tree), p, m->nofNeighbors);

      // neighbors are not sorted anymore,
      // let's find the nearest
      Point_ptr_with_transformed_distance nearest = search_nearest(search);
      // if p is far away the input point cloud, start with the closest point.
      if (!isValid(nearest,p))
      {
        p = *nearest.first;
        n =  nearest.first->normal();
        delta2 = nearest.second;
      }
      else
      {
        fit(search);

        Point oldP = p;
        if (m->as.state==AlgebraicSphere::SPHERE)
        {
          // projection onto a sphere.
          Vector dir = normalize(sub(source,m->as.center));
          p = add(m->as.center,mul(m->as.radius,dir));
          FT flipN = m->as.u4<0. ? -1. : 1.;
          if (!isValid(nearest,p))
          {
            // if the closest intersection is far away the input points,
            // then we take the other one.
            p = add(m->as.center,mul(-m->as.radius,dir));
            flipN = -flipN;
          }
          n = mul(flipN,dir);

          if (!isValid(nearest,p))
          {
            std::cout << "Invalid projection\n";
          }
        }
        else if (m->as.state==AlgebraicSphere::PLANE)
        {
          // projection onto a plane.
          p = sub(source, mul(dot(m->as.normal,source-CGAL::ORIGIN)+m->as.d,m->as.normal));
        }
        else
        {
          // iterative projection onto the algebraic sphere.
          p = m->as.iProject(source);
        }

        Vector diff = sub(oldP,p);
        delta2 = dot(diff,diff);
      }

    } while ( ((++countIter)<maxNofIterations) && (delta2<m->sqError) );
  }

  inline static FT dot(const Vector& a, const Vector& b) {
    return a.x()*b.x() + a.y()*b.y() + a.z()*b.z();
  }
  inline static FT length(const Vector& a) {
    return sqrt(dot(a,a));
  }
  inline static Vector mul(FT s, const Vector& p) {
    return Vector(p.x()*s, p.y()*s, p.z()*s);
  }
  inline static Point add(FT s, const Point& p) {
    return Point(p.x()+s, p.y()+s, p.z()+s);
  }
  inline static Point add(const Point& a, const Vector& b) {
    return Point(a.x()+b.x(), a.y()+b.y(), a.z()+b.z());
  }
  inline static Vector add(const Vector& a, const Vector& b) {
    return Vector(a.x()+b.x(), a.y()+b.y(), a.z()+b.z());
  }
  inline static Point sub(const Point& a, const Vector& b) {
    return Point(a.x()-b.x(), a.y()-b.y(), a.z()-b.z());
  }
  inline static Vector sub(const Point& a, const Point& b) {
    return Vector(a.x()-b.x(), a.y()-b.y(), a.z()-b.z());
  }
  inline static Vector normalize(const Vector& p) {
    FT s = 1. / length(p);
    return mul(s,p);
  }
  inline static Vector cross(const Vector& a, const Vector& b) {
    return Vector(a.y()*b.z() - a.z()*b.y(),
      a.z()*b.x() - a.x()*b.z(),
      a.x()*b.y() - a.y()*b.x());
  }

private:

  struct AlgebraicSphere {
    FT u0, u4;
    Vector u13;
    enum State {UNDETERMINED=0,PLANE=1,SPHERE=2};
    State state;
    Point center;
    FT radius;
    Vector normal;
    FT d;

    AlgebraicSphere() : state(UNDETERMINED) {}

    /** Converts the algebraic sphere to an explicit sphere or plane.
    */
    void finalize(void) {
      if (fabs(u4)>1e-9)
      {
        state = SPHERE;
        FT b = 1./u4;
        center = CGAL::ORIGIN + mul(-0.5*b,u13);
        radius = sqrt(dot(center - CGAL::ORIGIN,center - CGAL::ORIGIN) - b*u0);
      }
      else if (u4==0.)
      {
        state = PLANE;
        FT s = 1./length(u13);
        normal = mul(s,u13);
        d = u0*s;
      }
      else
      {
        state = UNDETERMINED;
      }
    }

    /** Compute the Euclidean distance between the algebraic surface and a point.
    */
    FT euclideanDistance(const Point& p) {
      if (state==SPHERE)
      {
        FT aux = length(sub(center,p)) - radius;
        if (u4<0.)
          aux = -aux;
        return aux;
      }

      if (state==PLANE)
        return dot(p - CGAL::ORIGIN,normal) + d;

      // else, tedious case, fall back to an iterative method:
      return iEuclideanDistance(p);
    }

    /** Euclidean distance via an iterative projection procedure.
    This is an optimized version of distance(x,iProject(x)).
    */
    inline FT iEuclideanDistance(const Point& x) const
    {
      FT d = 0.;
      Vector grad;
      Vector dir = add(mul(2.*u4,x-CGAL::ORIGIN),u13);
      FT ilg = 1./length(dir);
      dir = mul(ilg,dir);
      FT ad = u0 + dot(u13,x-CGAL::ORIGIN) + u4 * dot(x-CGAL::ORIGIN,x-CGAL::ORIGIN);
      FT delta = -ad*(ilg<1.?ilg:1.);
      Point p = add(x, mul(delta,dir));
      d += delta;
      for (int i=0 ; i<5 ; ++i)
      {
        grad = add(mul(2.*u4,p-CGAL::ORIGIN),u13);
        ilg = 1./length(grad);
        delta = -(u0 + dot(u13,p-CGAL::ORIGIN) + u4 * dot(p-CGAL::ORIGIN,p-CGAL::ORIGIN))*(ilg<1.?ilg:1.);
        p = add(p,mul(delta,dir));
        d += delta;
      }
      return -d;
    }

    /** Iterative projection.
    */
    inline Point iProject(const Point& x) const
    {
      Vector grad;
      Vector dir = add(mul(2.*u4,x-CGAL::ORIGIN),u13);
      FT ilg = 1./length(dir);
      dir = mul(ilg,dir);
      FT ad = u0 + dot(u13,x-CGAL::ORIGIN) + u4 * dot(x-CGAL::ORIGIN,x-CGAL::ORIGIN);
      FT delta = -ad*(ilg<1.?ilg:1.);
      Point p = add(x, mul(delta,dir));
      for (int i=0 ; i<5 ; ++i)
      {
        grad = add(mul(2.*u4,p-CGAL::ORIGIN),u13);
        ilg = 1./length(grad);
        delta = -(u0 + dot(u13,p-CGAL::ORIGIN) + u4 * dot(p-CGAL::ORIGIN,p-CGAL::ORIGIN))*(ilg<1.?ilg:1.);
        p = add(p,mul(delta,dir));
      }
      return p;
    }
  };

  /// Find a random point inside the surface.
  void find_inner_point()
  {
    m->inner_point = CGAL::ORIGIN;
    FT min_f = 1e38;

    // Try random points until we find a point / value < 0
    Point center = m->bounding_sphere.center();
    FT radius = sqrt(m->bounding_sphere.squared_radius());
    CGAL::Random_points_in_sphere_3<Point> rnd(radius);
    while (min_f > 0)
    {
      // Create random point in bounding sphere
      Point p = center + (*rnd++ - CGAL::ORIGIN);
      FT value = (*this)(p);
      if(value < min_f)
      {
        m->inner_point = p;
        min_f = value;
      }
    }
  }

// Data members
private:

  struct Private
  {
    Private()
      : tree(NULL), count(1)
    {}

    ~Private()
    {
      delete tree; tree = NULL;
    }

    Tree* tree;
    std::vector<KdTreeElement> treeElements;
    std::vector<FT> radii;
    Sphere bounding_sphere; // Points' bounding sphere
    FT sqError; // Dichotomy error when projecting point (squared)
    unsigned int nofNeighbors; // Number of nearest neighbors
    Point inner_point; // Point inside the surface
    mutable AlgebraicSphere as;
    mutable Point_ptr_with_transformed_distance cached_nearest_neighbor;
    int count; // reference counter
  };

  Private* m; // smart pointer to data

}; // end of APSS_reconstruction_function


CGAL_END_NAMESPACE

#endif // CGAL_APSS_RECONSTRUCTION_FUNCTION_H