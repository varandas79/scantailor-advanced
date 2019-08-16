// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef DEWARPING_CYLINDRICAL_SURFACE_DEWARPER_H_
#define DEWARPING_CYLINDRICAL_SURFACE_DEWARPER_H_

#include <QLineF>
#include <QPointF>
#include <boost/array.hpp>
#include <utility>
#include <vector>
#include "ArcLengthMapper.h"
#include "HomographicTransform.h"
#include "PolylineIntersector.h"

namespace dewarping {
class CylindricalSurfaceDewarper {
 public:
  class State {
    friend class CylindricalSurfaceDewarper;

   private:
    PolylineIntersector::Hint m_intersectionHint1;
    PolylineIntersector::Hint m_intersectionHint2;
    ArcLengthMapper::Hint m_arcLengthHint;
  };


  struct Generatrix {
    QLineF imgLine;
    HomographicTransform<1, double> pln2img;

    Generatrix(const QLineF& img_line, const HomographicTransform<1, double>& H) : imgLine(img_line), pln2img(H) {}
  };

  /**
   * \param depth_perception The distance from the camera to the plane formed
   *        by two outer generatrixes, in some unknown units :)
   *        This model assumes that plane is perpendicular to the camera direction.
   *        In practice, just use values between 1 and 3.
   */
  CylindricalSurfaceDewarper(const std::vector<QPointF>& img_directrix1,
                             const std::vector<QPointF>& img_directrix2,
                             double depth_perception);

  /**
   * \brief Returns the arc length of a directrix, assuming its
   *        chord length is one.
   */
  double directrixArcLength() const { return m_directrixArcLength; }

  Generatrix mapGeneratrix(double crv_x, State& state) const;

  /**
   * Transforms a point from warped image coordinates
   * to dewarped normalized coordinates.  See comments
   * in the beginning of the *.cpp file for more information
   * about coordinate systems we work with.
   */
  QPointF mapToDewarpedSpace(const QPointF& img_pt) const;

  /**
   * Transforms a point from dewarped normalized coordinates
   * to warped image coordinates.  See comments in the beginning
   * of the *.cpp file for more information about coordinate
   * systems we owork with.
   */
  QPointF mapToWarpedSpace(const QPointF& crv_pt) const;

 private:
  class CoupledPolylinesIterator;

  static HomographicTransform<2, double> calcPlnToImgHomography(const std::vector<QPointF>& img_directrix1,
                                                                const std::vector<QPointF>& img_directrix2);

  static double calcPlnStraightLineY(const std::vector<QPointF>& img_directrix1,
                                     const std::vector<QPointF>& img_directrix2,
                                     HomographicTransform<2, double> pln2img,
                                     HomographicTransform<2, double> img2pln);

  static HomographicTransform<2, double> fourPoint2DHomography(
      const boost::array<std::pair<QPointF, QPointF>, 4>& pairs);

  static HomographicTransform<1, double> threePoint1DHomography(
      const boost::array<std::pair<double, double>, 3>& pairs);

  void initArcLengthMapper(const std::vector<QPointF>& img_directrix1, const std::vector<QPointF>& img_directrix2);

  HomographicTransform<2, double> m_pln2img;
  HomographicTransform<2, double> m_img2pln;
  double m_depthPerception;
  double m_plnStraightLineY;
  double m_directrixArcLength;
  ArcLengthMapper m_arcLengthMapper;
  PolylineIntersector m_imgDirectrix1Intersector;
  PolylineIntersector m_imgDirectrix2Intersector;
};
}  // namespace dewarping
#endif  // ifndef DEWARPING_CYLINDRICAL_SURFACE_DEWARPER_H_
