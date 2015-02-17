#include "CCTag.hpp"

#include "global.hpp"
#include "dataSerialization.hpp"
#include "algebra/invert.hpp"
#include "geometry/Ellipse.hpp"
#include "statistic/statistic.hpp"
#include "algebra/matrix/operation.hpp"
#include "geometry/distance.hpp"
#include "optimization/conditioner.hpp"
#include "viewGeometry/homography.hpp"
#include "viewGeometry/2DTransform.hpp"

#ifdef WITH_CMINPACK
#include <cminpack.h>
#endif

#include <opencv2/core/core_c.h>

#include <boost/foreach.hpp>
#include <boost/timer.hpp>
#include <boost/array.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/numeric/ublas/expression_types.hpp>
#include <boost/numeric/ublas/functional.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_expression.hpp>

#include <cstddef>
#include <cmath>
#include <iomanip>

namespace rom {
    namespace vision {
        namespace marker {

            namespace ublas = boost::numeric::ublas;
            namespace optimization = rom::numerical::optimization;

            // todo@Lilian : used in the initRadiusRatio called in the CCTag constructor. Need to be changed while reading the CCTagBank build from the textFile.
            const boost::array<double, 5> CCTag::_radiusRatiosInit = {
                (29.0 / 9.0),
                (29.0 / 13.0),
                (29.0 / 17.0),
                (29.0 / 21.0),
                (29.0 / 25.0)
            };

            void CCTag::condition(const rom::numerical::BoundedMatrix3x3d & mT, const rom::numerical::BoundedMatrix3x3d & mInvT) {
                using namespace rom::numerical::geometry;

                // Condition outer ellipse
                _outerEllipse = _outerEllipse.transform(mInvT);
                rom::numerical::normalizeDet1(_outerEllipse.matrix());

                // Condition each ellipses if they exist.

                BOOST_FOREACH(rom::numerical::geometry::Ellipse & ellipse, _ellipses) {
                    ellipse = ellipse.transform(mInvT);
                    rom::numerical::normalizeDet1(ellipse.matrix());
                }

                BOOST_FOREACH(std::vector<rom::Point2dN<double> > & pts, _points) {
                    rom::numerical::optimization::condition(pts, mT);
                }

                rom::numerical::optimization::condition(_centerImg, mT);
            }

            void CCTag::scale(const double s) {

                BOOST_FOREACH(std::vector< Point2dN<double> > &vp, _points) {

                    BOOST_FOREACH(Point2dN<double> &p, vp) {
                        p.setX(p.x() * s);
                        p.setY(p.y() * s);
                    }
                }

                _centerImg.setX(_centerImg.x() * s);
                _centerImg.setY(_centerImg.y() * s);

                _outerEllipse.setCenter(Point2dN<double>(_outerEllipse.center().x() * s,
                        _outerEllipse.center().y() * s));
                _outerEllipse.setA(_outerEllipse.a() * s);
                _outerEllipse.setB(_outerEllipse.b() * s);
            }

            void CCTag::serialize(boost::archive::text_oarchive & ar, const unsigned int version) {
                ar & BOOST_SERIALIZATION_NVP(_nCircles);
                ar & BOOST_SERIALIZATION_NVP(_id);
                ar & BOOST_SERIALIZATION_NVP(_pyramidLevel);
                ar & BOOST_SERIALIZATION_NVP(_scale);
                ar & BOOST_SERIALIZATION_NVP(_status);
                serializeEllipse(ar, _outerEllipse);
                serializeEllipse(ar, _rescaledOuterEllipse);
                serializeIdSet(ar, _idSet);
                serializeRadiusRatios(ar, _radiusRatios);
                ar & BOOST_SERIALIZATION_NVP(_quality);
                serializePoints(ar, _points);
                serializeEllipses(ar, _ellipses);
                serializeBoundedMatrix3x3d(ar, _mHomography);
                serializePoint(ar, _centerImg);
#ifdef CCTAG_STAT_DEBUG
                serializeFlowComponents(ar, _flowComponents);
#endif
            }

        }
    }
}