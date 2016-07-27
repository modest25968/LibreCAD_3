#include "cad/math/intersectionhandler.h"
using namespace lc;
using namespace maths;
#include <iostream>
std::vector<geo::Coordinate> Intersection::LineLine(const Equation& l1,
                                                    const Equation& l2) {
    std::vector<lc::geo::Coordinate> ret;
    const auto &m1 = l1.Matrix();
    const auto &m2 = l2.Matrix();
    Eigen::Matrix2d M;
    Eigen::Vector2d V;

    M << m1(2,0) + m1(0,2), m1(2,1) + m1(1,2),
            m2(2,0) + m2(0,2), m2(2,1) + m2(1,2);
    V << -m1(2,2), -m2(2,2);

    Eigen::Vector2d R = M.colPivHouseholderQr().solve(V);
    ret.emplace_back(R[0], R[1]);
    return ret;
}

std::vector<lc::geo::Coordinate> Intersection::LineQuad(const Equation& l1,
                                                        const Equation& q1) {
    auto &&tcoords = QuadQuad(l1.flipXY(), q1.flipXY());
    std::transform(tcoords.begin(), tcoords.end(), tcoords.begin(), [](const lc::geo::Coordinate &c)  { return std::move(c.flipXY()); });
    return tcoords;
}

std::vector<lc::geo::Coordinate> Intersection::QuadQuad(const Equation& l1,
                                                        const Equation& l2) {
    const auto &m1 = l1.Matrix();
    const auto &m2 = l2.Matrix();

    if (std::abs(m1(0, 0)) < LCTOLERANCE && std::abs(m1(0, 1)) < LCTOLERANCE
            &&
            std::abs(m2(0, 0)) < LCTOLERANCE && std::abs(m2(0, 1)) < LCTOLERANCE
            ) {
        if (std::abs(m1(1, 1)) < LCTOLERANCE && std::abs(m2(1, 1)) < LCTOLERANCE) {
            return LineLine(l1, l2);
        }
        return LineQuad(l1,l2);
    }

    std::vector<std::vector<double> >  ce(0);
    ce.push_back(l1.Coefficients());
    ce.push_back(l2.Coefficients());
    return Math::simultaneousQuadraticSolverFull(ce);
}


std::vector<geo::Coordinate> Intersection::BezierLine(
        geo::BB_CSPtr B, const geo::Vector& V) {

    std::vector<geo::Coordinate> ret;
    std::vector<double> roots;

    auto pts = B->getCP();

    if(pts.size()==3) {
        auto ml = geo::Vector(V.start() - V.start(), V.end() - V.start());
        auto rotate_angle = -ml.Angle1();
        auto cps = B->move(-V.start())->rotate(ml.start(), rotate_angle)->getCP();

        auto t2 = cps[0].y() - 2*cps[1].y() + cps[2].y();
        auto t1 = 2*(cps[1].y() - cps[0].y())/t2;
        auto coeff = cps[0].y()/t2;

        roots = lc::Math::quadraticSolver({t1, coeff});
    } else {
        auto ml = geo::Vector(V.start() - V.start(), V.end() - V.start());
        auto rotate_angle = -ml.Angle1();
        auto cps = B->move(-V.start())->rotate(ml.start(), rotate_angle)->getCP();

        auto t3 = -cps[0].y() + 3*cps[1].y() - 3*cps[2].y() + cps[3].y();
        auto t2 = (3*cps[0].y() - 6*cps[1].y() + 3*cps[2].y())/t3;
        auto t1 = (-3*cps[0].y() +3*cps[1].y())/t3;
        auto coeff = cps[0].y()/t3;

        roots = lc::Math::cubicSolver({t2, t1, coeff});
    }
    for(const auto &root : roots) {
        if(root > 0 && root < 1) {
            ret.push_back(B->DirectValueAt(root));
        }
    }

    return ret;
}


std::vector<geo::Coordinate> Intersection::BezierCircle(
        geo::BB_CSPtr B, const geo::Circle& C) {

    std::vector<geo::Coordinate> ret;

    // ((x0 (1-t)^2+ x1 (2 t - 2 t^2) + x2 t^2) - xC)^2 + ((y0 (1-t)^2+ y1 (2 t - 2 t^2) + y2 t^2) - yC)^2 = r2

    // (((a - 2b + c)t^2 + 2t(b-a) + a) - d)^2 + (((e - 2f + g)t^2 + 2t(f-e) + e) - h)^2 - r^2

    // Solving this for T will get the required intersection
    std::vector<double>roots;

    auto points = B->getCP();

    if(points.size()== 3) {

        auto r = C.radius();
        auto d = C.center().x();
        auto h = C.center().y();

        auto a = points.at(0).x();
        auto b = points.at(1).x();
        auto c = points.at(2).x();

        auto e = points.at(0).y();
        auto f = points.at(1).y();
        auto g = points.at(2).y();

        auto t4 = (g*g + (2*e - 4*f)*g + 4* f*f - 4* e * f + e*e + c*c + (2*a-4*b)*c + 4*b*b - 4*a*b + a*a);
        auto t3 = ((4*f - 4*e)*g - 8*f*f + 12*e*f - 4*e*e + (4*b - 4*a)*c - 8*b*b + 12*a*b -4*a*a)/t4;
        auto t2 = ((-2*g + 4*f -2*e)*h + 2*e*g + 4*f*f - 12*e*f + 6*e*e + (-2*c + 4*b - 2*a)*d + 2*a*c + 4*b*b -12*a*b + 6*a*a)/t4;
        auto t1 = ((4*e - 4*f)*h + 4*e*f - 4*e*e + (4*a - 4*b)*d + 4*a*b -4*a*a)/t4;
        auto coeff = (-r*r + h*h -2 *e*h + e*e + d*d - 2*a*d + a*a)/t4;

        roots = lc::Math::quarticSolver({t3, t2, t1, coeff});

    } else {

        std::cout << "bezier cubic becier cubiz";
        auto a = points.at(0).x();
        auto b = points.at(1).x();
        auto c = points.at(2).x();
        auto d = points.at(3).x();

        auto e = points.at(0).y();
        auto f = points.at(1).y();
        auto g = points.at(2).y();
        auto h = points.at(3).x();

        auto r = C.radius();
        auto m = C.center().x();
        auto n = C.center().y();

        auto a_sq = a*a;
        auto b_sq = b*b;
        auto c_sq = c*c;
        auto d_sq = d*d;
        auto e_sq = e*e;
        auto f_sq = f*f;
        auto g_sq = g*g;
        auto h_sq = h*h;
        auto m_sq = m*m;
        auto n_sq = n*n;
        auto r_sq = r*r;

//        auto a = points.at(0).x();
//        auto b = points.at(1).x();
//        auto c = points.at(2).x();
//        auto d = points.at(3).x();

//        auto f = points.at(0).y();
//        auto g = points.at(1).y();
//        auto h = points.at(2).y();
//        auto i = points.at(3).x();

//        auto r = C.radius();
//        auto z = C.center().x();
//        auto y = C.center().y();

//        auto a_sq = a*a;
//        auto b_sq = b*b;
//        auto c_sq = c*c;
//        auto d_sq = d*d;

//        auto f_sq = f*f;
//        auto g_sq = g*g;
//        auto h_sq = h*h;
//        auto i_sq = i*i;

//        auto z_sq = z*z;
//        auto y_sq = y*y;
//        auto r_sq = r*r;

        auto t6 = (a_sq - 6*a*b + 6*a*c - 2*a*d + 9*b_sq - 18*b*c + 6*b*d + 9*c_sq - 6*c*d + d_sq + e_sq - 6*e*f + 6*e*g - 2*e*h + 9*f_sq - 18*f*g + 6*f*h + 9*g_sq - 6*g*h + h_sq);
        auto t5 = ( - 6*a_sq + 30*a*b - 24*a*c + 6*a*d - 36*b_sq + 54*b*c - 12*b*d - 18*c_sq + 6*c*d - 6*e_sq + 30*e*f - 24*e*g + 6*e*h - 36*f_sq + 54*f*g - 12*f*h - 18*g_sq + 6*g*h)/t6;
        auto t4 = (15*a_sq - 60*a*b + 36*a*c - 6*a*d + 54*b_sq - 54*b*c + 6*b*d + 9*c_sq + 15*e_sq - 60*e*f + 36*e*g - 6*e*h + 54*f_sq - 54*f*g + 6*f*h + 9*g_sq)/t6;
        auto t3 = ( - 20*a_sq + 60*a*b - 24*a*c + 2*a*d + 2*a*m - 36*b_sq + 18*b*c - 6*b*m + 6*c*m - 2*d*m - 20*e_sq + 60*e*f - 24*e*g + 2*e*h + 2*e*n - 36*f_sq + 18*f*g - 6*f*n + 6*g*n - 2*h*n)/t6;
        auto t2 = (15*a_sq - 30*a*b + 6*a*c - 6*a*m + 9*b_sq + 12*b*m - 6*c*m + 15*e_sq - 30*e*f + 6*e*g - 6*e*n + 9*f_sq + 12*f*n - 6*g*n)/t6;
        auto t = ( - 6*a_sq + 6*a*b + 6*a*m - 6*b*m - 6*e_sq + 6*e*f + 6*e*n - 6*f*n)/t6;
        auto coeff = (a_sq - 2*a*m + e_sq - 2*e*n + m_sq + n_sq - r_sq)/t6;

//        auto t6 = a_sq - 6*a*b + 6*a*c - 2*a*d + 9*b_sq - 18*b*c + 6*b*d + 9*c_sq - 6*c*d + d_sq + f_sq - 6*f*g + 6*f*h - 2*f*i + 9*g_sq - 18*g*h + 6*g*i + 9*h_sq - 6*h*i + i_sq;
//        auto t5 = -6*a_sq + 30*a*b - 24*a*c + 6*a*d - 36*b_sq + 54*b*c - 12*b*d - 18*c_sq + 6*c*d - 6*f_sq + 30*f*g - 24*f*h + 6*f*i - 36*g_sq + 54*g*h - 12*g*i - 18*h_sq + 6*h*i;
//        auto t4 = 15*a_sq - 60*a*b + 36*a*c - 6*a*d + 54*b_sq - 54*b*c + 6*b*d + 9*c_sq + 15*f_sq - 60*f*g + 36*f*h - 6*f*i + 54*g_sq - 54*g*h + 6*g*i + 9*h_sq;
//        auto t3 = -20*a_sq + 60*a*b - 24*a*c + 2*a*d + 2*a*z -36*b_sq + 18*b*c - 6*b*z + 6*c*z - 2*d*z - 20*f_sq + 60*f*g -24*f*h + 2*f*i + 2*f*y - 36*g_sq + 18*g*h - 6*g*y + 6*h*y - 2*i*y;
//        auto t2 = 15*a_sq - 30*a*b + 6*a*c - 6*a*z + 9*b_sq + 12*b*z - 6*c*z + 15*f_sq - 30*f*g - 6*f*h - 6*f*y + 9*g_sq + 12*g*y - 6*h*y;
//        auto t =  - 6*a_sq + 6*a*b + 6*a*z - 6*b*z - 6*f_sq + 6*f*g * 6*f*y - 6*g*y;
//        auto coeff =a_sq  - 2*a*z + f_sq - 2*f*y - r_sq + y_sq + z_sq;
//        roots = lc::Math::sexticSolver({t6, t5, t4, t3, t2, t, coeff});

//        auto t6 = (a_sq - 6*a*b + 6*a*c - 2*a*d + 9*b_sq - 18*b*c + 6*b*d + 9*c_sq - 6*c*d + d_sq + f_sq - 6*f*g + 6*f*h - 2*f*i + 9*g_sq - 18*g*h + 6*g*i + 9*h_sq - 6*h*i + i_sq);
//        auto t5 = (-6*a_sq + 30*a*b - 24*a*c + 6*a*d - 36*b_sq + 54*b*c - 12*b*d - 18*c_sq + 6*c*d - 6*f_sq + 30*f*g - 24*f*h + 6*f*i - 36*g_sq + 54*g*h - 12*g*i - 18*h_sq + 6*h*i)/t6;
//        auto t4 = (15*a_sq - 60*a*b + 36*a*c - 6*a*d + 54*b_sq - 54*b*c + 6*b*d + 9*c_sq + 15*f_sq - 60*f*g + 36*f*h - 6*f*i + 54*g_sq - 54*g*h + 6*g*i + 9*h_sq)/t6;
//        auto t3 = (-20*a_sq + 60*a*b - 24*a*c + 2*a*d + 2*a*z -36*b_sq + 18*b*c - 6*b*z + 6*c*z - 2*d*z - 20*f_sq + 60*f*g -24*f*h + 2*f*i + 2*f*y - 36*g_sq + 18*g*h - 6*g*y + 6*h*y - 2*i*y)/t6;
//        auto t2 = (15*a_sq - 30*a*b + 6*a*c - 6*a*z + 9*b_sq + 12*b*z - 6*c*z + 15*f_sq - 30*f*g - 6*f*h - 6*f*y + 9*g_sq + 12*g*y - 6*h*y)/t6;
//        auto t =  (- 6*a_sq + 6*a*b + 6*a*z - 6*b*z - 6*f_sq + 6*f*g * 6*f*y - 6*g*y)/t6;
//        auto coeff = (a_sq  - 2*a*z + f_sq - 2*f*y - r_sq + y_sq + z_sq)/t6;

        roots = lc::Math::sexticSolver({t5, t4, t3, t2, t, coeff});

    }

    std::cout << roots.size() << std::endl;

    for(const auto &root : roots) {
        if(root > 0 && root < 1) {
            ret.push_back(B->DirectValueAt(root));
        }
    }
    return ret;
}


std::vector<geo::Coordinate> Intersection::BezierArc(
        geo::BB_CSPtr B, const geo::Arc& A) {

    // BezierCircle Intersection

    // Check intersection points are on Arc.

    std::vector<geo::Coordinate> ret;
    const auto &points = BezierCircle(B, geo::Circle(A.center(), A.radius()));

    for(const auto & pt : points) {
        if(A.isAngleBetween(pt.angle())) {
            ret.push_back(pt);
        }
    }
    return ret;
}


std::vector<geo::Coordinate> Intersection::BezierEllipse(
        geo::BB_CSPtr B, const geo::Ellipse& E) {
    std::vector<double> roots;
    std::vector<geo::Coordinate> arc_ret, ret;


    auto C = geo::Ellipse(E.center() - E.center(), E.majorP(), E.minorRadius(), E.startAngle(), E.endAngle())
            .georotate(geo::Coordinate(0,0), -E.getAngle())
            .geoscale(geo::Coordinate(0,0), geo::Coordinate(1/E.ratio(),1));

    auto bez = B->move(-E.center())
            ->rotate(geo::Coordinate(0,0), E.getAngle())
            ->scale(geo::Coordinate(0,0), geo::Coordinate(1/E.ratio(),1));


    auto points = B->getCP();

    if(points.size()== 3) {
        auto r = C.minorRadius();
        auto d = C.center().x();
        auto h = C.center().y();

        auto a = points.at(0).x();
        auto b = points.at(1).x();
        auto c = points.at(2).x();

        auto e = points.at(0).y();
        auto f = points.at(1).y();
        auto g = points.at(2).y();

        auto t4 = (g*g + (2*e - 4*f)*g + 4* f*f - 4* e * f + e*e + c*c + (2*a-4*b)*c + 4*b*b - 4*a*b + a*a);
        auto t3 = ((4*f - 4*e)*g - 8*f*f + 12*e*f - 4*e*e + (4*b - 4*a)*c - 8*b*b + 12*a*b -4*a*a)/t4;
        auto t2 = ((-2*g + 4*f -2*e)*h + 2*e*g + 4*f*f - 12*e*f + 6*e*e + (-2*c + 4*b - 2*a)*d + 2*a*c + 4*b*b -12*a*b + 6*a*a)/t4;
        auto t1 = ((4*e - 4*f)*h + 4*e*f - 4*e*e + (4*a - 4*b)*d + 4*a*b -4*a*a)/t4;
        auto coeff = (-r*r + h*h -2 *e*h + e*e + d*d - 2*a*d + a*a)/t4;

        roots = lc::Math::quarticSolver({t3, t2, t1, coeff});

    } else {
//        auto a = points.at(0).x();
//        auto b = points.at(1).x();
//        auto c = points.at(2).x();
//        auto d = points.at(3).x();

//        auto e = points.at(0).y();
//        auto f = points.at(1).y();
//        auto g = points.at(2).y();
//        auto h = points.at(3).x();

//        auto r = C.minorRadius();
//        auto m = C.center().x();
//        auto n = C.center().y();

//        auto a_sq = a*a;
//        auto b_sq = b*b;
//        auto c_sq = c*c;
//        auto d_sq = d*d;
//        auto e_sq = e*e;
//        auto f_sq = f*f;
//        auto g_sq = g*g;
//        auto h_sq = h*h;
//        auto m_sq = m*m;
//        auto n_sq = n*n;
//        auto r_sq = r*r;

//        auto t6 = (a_sq - 6*a*b + 6*a*c - 2*a*d + 9*b_sq - 18*b*c + 6*b*d + 9*c_sq - 6*c*d + d_sq + e_sq - 6*e*f + 6*e*g - 2*e*h + 9*f_sq - 18*f*g + 6*f*h + 9*g_sq - 6*g*h + h_sq);
//        auto t5 = ( - 6*a_sq + 30*a*b - 24*a*c + 6*a*d - 36*b_sq + 54*b*c - 12*b*d - 18*c_sq + 6*c*d - 6*e_sq + 30*e*f - 24*e*g + 6*e*h - 36*f_sq + 54*f*g - 12*f*h - 18*g_sq + 6*g*h)/t6;
//        auto t4 = (15*a_sq - 60*a*b + 36*a*c - 6*a*d + 54*b_sq - 54*b*c + 6*b*d + 9*c_sq + 15*e_sq - 60*e*f + 36*e*g - 6*e*h + 54*f_sq - 54*f*g + 6*f*h + 9*g_sq)/t6;
//        auto t3 = ( - 20*a_sq + 60*a*b - 24*a*c + 2*a*d + 2*a*m - 36*b_sq + 18*b*c - 6*b*m + 6*c*m - 2*d*m - 20*e_sq + 60*e*f - 24*e*g + 2*e*h + 2*e*n - 36*f_sq + 18*f*g - 6*f*n + 6*g*n - 2*h*n)/t6;
//        auto t2 = (15*a_sq - 30*a*b + 6*a*c - 6*a*m + 9*b_sq + 12*b*m - 6*c*m + 15*e_sq - 30*e*f + 6*e*g - 6*e*n + 9*f_sq + 12*f*n - 6*g*n)/t6;
//        auto t1 = ( - 6*a_sq + 6*a*b + 6*a*m - 6*b*m - 6*e_sq + 6*e*f + 6*e*n - 6*f*n)/t6;
//        auto t = (a_sq - 2*a*m + e_sq - 2*e*n + m_sq + n_sq - r_sq)/t6;

//        roots = lc::Math::sexticSolver({t5, t4, t3, t2, t1, t});
    }

    for(const auto &root : roots) {
        if(root > 0 && root < 1) {
            ret.push_back(B->DirectValueAt(root));
        }
    }
    if(E.isArc()) {
        for(const auto &r: ret) {
            if(E.isAngleBetween(r.angle())) {
                arc_ret.push_back(r);
            }
        }
        return arc_ret;
    }
    return ret;
}

std::vector<geo::Coordinate> Intersection::BezierBezier(
        geo::BB_CSPtr B1, geo::BB_CSPtr B2) {
    std::vector<geo::Coordinate> ret;
    BezBez(B1,B2, ret);
    return ret;
}

void Intersection::BezBez(const geo::BB_CSPtr B1,const geo::BB_CSPtr B2, std::vector<geo::Coordinate>&ret) {
    auto bb1 = B1->boundingBox();
    auto bb2 = B2->boundingBox();

    if(!bb1.overlaps(bb2)) {
        return;
    }

    if(bb1.height() + bb2.height() <= BBHEURISTIC2 && bb1.width() + bb2.width() <= BBHEURISTIC2) {
        ret.push_back(B1->getCP().at(1));
        return;
    }

    auto b1split = B1->splitHalf();
    auto b2split = B2->splitHalf();
    BezBez(b1split[0], b2split[0], ret);
    BezBez(b1split[1], b2split[0], ret);
    BezBez(b1split[0], b2split[1], ret);
    BezBez(b1split[1], b2split[1], ret);
}
