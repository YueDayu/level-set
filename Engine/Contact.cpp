#include "Contact.h"
#include "vmath.h"
#include "Rigid_Geometry.h"
#include "Solver.h"
using namespace SimLib;

void Contact::contact_handling(std::vector<Contact>& contacts) {
    ARRAY<1,double> b((int)contacts.size() * 3);
    Vector3d r_v(4.3284, 2.3850, 3.2859);
    r_v.normalize();
    for (int i = 0; i < contacts.size(); ++i) {
        Contact& c = contacts[i];
        c.t1 = c.n.crossProduct(Vector3d(1.6947, 2.1259, 1.7465));
        c.t1.normalize();
        c.t2 = c.n.crossProduct(c.t1);
        Vector3d u = c.a->v + c.a->w * c.ra - c.b->v - c.b->w * c.rb;
        c.u = u - c.n * u.dotProduct(c.n);
        c.support = 0;
        Rigid_Geometry *A = c.a, *B = c.b;
        Vector3d &n = c.n, &ra = c.ra, &rb = c.rb;
        Vector3d &f_a = A->f, &f_b = B->f, &t_a = A->M, &t_b = B->M;
        double m1 = A->nailed ? 0 : 1 / A->mass;
        double m2 = B->nailed ? 0 : 1 / B->mass;
        Matrix3d r1 = A->rotation.rotMatrix();
        Matrix3d r2 = B->rotation.rotMatrix();
        Matrix3d J1 = A->nailed ? Matrix3d::createScale(0, 0, 0) : r1 * A->J * r1.transpose();
        Matrix3d J2 = B->nailed ? Matrix3d::createScale(0, 0, 0) : r2 * B->J * r2.transpose();
        Vector3d a_ext_part = f_a * m1 + ((J1 * t_a).crossProduct(ra));
        Vector3d b_ext_part = f_b * m2 + ((J2 * t_b).crossProduct(rb));
        Vector3d a_vel_part = A->w.crossProduct(A->w.crossProduct(ra)) + (J1 * (r1 * A->J0 * r1.transpose() * A->w)).crossProduct(ra);
        Vector3d b_vel_part = B->w.crossProduct(B->w.crossProduct(rb)) + (J2 * (r2 * B->J0 * r2.transpose() * B->w)).crossProduct(rb);
        double k1 = n.dotProduct(((a_ext_part + a_vel_part) - (b_ext_part + b_vel_part)));
        double k2 = 2 * B->w.crossProduct(n).dotProduct(A->v + A->w.crossProduct(ra) - B->v - B->w.crossProduct(rb));
        double k3 = c.t1.dotProduct(((a_ext_part + a_vel_part) - (b_ext_part + b_vel_part)));
        double k4 = 2 * B->w.crossProduct(c.t1).dotProduct(A->v + A->w.crossProduct(ra) - B->v - B->w.crossProduct(rb));
        double k5 = c.t2.dotProduct(((a_ext_part + a_vel_part) - (b_ext_part + b_vel_part)));
        double k6 = 2 * B->w.crossProduct(c.t2).dotProduct(A->v + A->w.crossProduct(ra) - B->v - B->w.crossProduct(rb));
        b(i * 3 + 1) = k1 + k2;
        b(i * 3 + 2) = k3 + k4;
        b(i * 3 + 3) = k5 + k6;
    }
    ARRAY<2, double> a((int)contacts.size() * 3, (int)contacts.size() * 3);
    for (int i = 0; i < contacts.size(); ++i) {
        Contact &ci = contacts[i];
        // Force LCP
        for (int j = 0; j < contacts.size(); ++j) {
            Contact &cj = contacts[j];
            if (ci.a != cj.a && ci.b != cj.b && ci.a != cj.b && ci.b != cj.a) {
                continue;
            }
            Rigid_Geometry *A = ci.a, *B = ci.b;
            Vector3d &ni = ci.n, &nj = cj.n, &pi = ci.p, &pj = cj.p, &ra = ci.ra, &rb = ci.rb;
            ci.ra = pi - ci.a->x;
            ci.rb = pi - ci.b->x;
            cj.ra = pj - cj.a->x;
            cj.rb = pj - cj.b->x;
            Vector3d force_a, force_b, torque_a, torque_b;
            Vector3d force_t1_a, force_t1_b, torque_t1_a, torque_t1_b;
            Vector3d force_t2_a, force_t2_b, torque_t2_a, torque_t2_b;
            if (cj.a == ci.a) {
                force_a = nj;
                torque_a = cj.ra.crossProduct(nj);
                force_t1_a = cj.t1;
                torque_t1_a = cj.ra.crossProduct(cj.t1);
                force_t2_a = cj.t2;
                torque_t2_a = cj.ra.crossProduct(cj.t2);
            } else {
                if (cj.b == ci.a) {
                    force_a = -nj;
                    torque_a = cj.rb.crossProduct(-nj);
                    force_t1_a = -cj.t1;
                    torque_t1_a = cj.ra.crossProduct(-cj.t1);
                    force_t2_a = -cj.t2;
                    torque_t2_a = cj.ra.crossProduct(-cj.t2);
                }
            }
            if (cj.a == ci.b) {
                force_b = nj;
                torque_b = cj.ra.crossProduct(nj);
                force_t1_b = cj.t1;
                torque_t1_b = cj.ra.crossProduct(cj.t1);
                force_t2_b = cj.t2;
                torque_t2_b = cj.ra.crossProduct(cj.t2);
            } else {
                if (cj.b == ci.b) {
                    force_b = -nj;
                    torque_b = cj.rb.crossProduct(-nj);
                    force_t1_b = -cj.t1;
                    torque_t1_b = cj.ra.crossProduct(-cj.t1);
                    force_t2_b = -cj.t2;
                    torque_t2_b = cj.ra.crossProduct(-cj.t2);
                }
            }
            double m1 = A->nailed ? 0 : 1 / A->mass;
            double m2 = B->nailed ? 0 : 1 / B->mass;
            Matrix3d r1 = A->rotation.rotMatrix();
            Matrix3d r2 = B->rotation.rotMatrix();
            Matrix3d J1 = A->nailed ? Matrix3d::createScale(0, 0, 0) : r1 * A->J * r1.transpose();
            Matrix3d J2 = B->nailed ? Matrix3d::createScale(0, 0, 0) : r2 * B->J * r2.transpose();
            Vector3d a_linear = force_a * m1, a_angular = (J1 * torque_a).crossProduct(ra);
            Vector3d b_linear = force_b * m2, b_angular = (J2 * torque_b).crossProduct(rb);
            Vector3d para = (a_linear + a_angular) - (b_linear + b_angular);
            a(i * 3 + 1,j * 3 + 1) = ni.dotProduct(para);
            a(i * 3 + 2,j * 3 + 1) = ci.t1.dotProduct(para);
            a(i * 3 + 3,j * 3 + 1) = ci.t2.dotProduct(para);
            a_linear = force_t1_a * m1, a_angular = (J1 * torque_t1_a).crossProduct(ra);
            b_linear = force_t1_b * m2, b_angular = (J2 * torque_t1_b).crossProduct(rb);
            para = (a_linear + a_angular) - (b_linear + b_angular);
            a(i * 3 + 1,j * 3 + 2) = ni.dotProduct((a_linear + a_angular) - (b_linear + b_angular));
            a(i * 3 + 2,j * 3 + 2) = ci.t1.dotProduct(para);
            a(i * 3 + 3,j * 3 + 2) = ci.t2.dotProduct(para);
            a_linear = force_t2_a * m1, a_angular = (J1 * torque_t2_a).crossProduct(ra);
            b_linear = force_t2_b * m2, b_angular = (J2 * torque_t2_b).crossProduct(rb);
            para = (a_linear + a_angular) - (b_linear + b_angular);
            a(i * 3 + 1,j * 3 + 3) = ni.dotProduct((a_linear + a_angular) - (b_linear + b_angular));
            a(i * 3 + 2,j * 3 + 3) = ci.t1.dotProduct(para);
            a(i * 3 + 3,j * 3 + 3) = ci.t2.dotProduct(para);
        }
    }
    ARRAY<1, double> c = b;
    if (contacts[0].a->name[2] == 'w' && contacts[0].a->v.length() > 0) {
        contacts = contacts;
    }
    ARRAY<1, double> X(c.dim(1));
    for (int i = 0; i < 2g00; ++i) {
        bool flag = false;
        for (int j = 0; j < contacts.size(); ++j) {
            ARRAY<1, double> f(3);
            f(1) = c(j*3+1) == 0 ? 0 : -c(j * 3 + 1) / a(j * 3 + 1, j * 3 + 1);
            if (f(1) < 0)
                f(1) = 0;
            if (contacts[j].u.length() != 0) {
                Vector3d fr = (-contacts[j].u);
                fr.normalize();
                fr *= contacts[j].mu;
                double x1 = fr.dotProduct(contacts[j].t1);
                double x2 = fr.dotProduct(contacts[j].t2);
                f(1) = c(j * 3 + 1) == 0 ? 0 : -c(j * 3 + 1) / (a(j * 3 + 1,j * 3 + 1) + x1 * a(j * 3 + 1,j * 3 + 2) + x2 * a(j * 3 + 1, j * 3 + 3));
                if (f(1) < 0)
                    f(1) = 0;
                f(2) = x1 * f(1);
                f(3) = x2 * f(1);
            } else {
                if (c(j * 3 + 1) == 0) {
                    f.Fill(0);
                } else {
                    Matrix3d m;
                    for (int k = 1; k <= 3; ++k)
                        for (int l = 1; l <= 3; ++l)
                            m.at(l-1,k-1) = a(j * 3 + k, j * 3 + l);
                    m = m.inverse();
                    Vector3d v = m * Vector3d(-c(j * 3 + 1), -c(j * 3 + 2), -c(j * 3 + 3));
                    if (v[0] > 1e4) {
                        v = v;
                    }
                    for (int k = 1; k <= 3; ++k) {
                        double test = c(j * 3 + k);
                        for (int l = 1; l <= 3; ++l)
                            test += a(j * 3 + k, j * 3 + l) * v[l - 1];
                        test = test;
                    }
                    f(1) = v[0]; f(2) = v[1]; f(3) = v[2];
                    double len = sqrt(f(2) * f(2) + f(3) * f(3)) / (f(1) * contacts[j].mu);
                    if (len > 1) {
                        while (true) {
                            f(2) /= len;
                            f(3) /= len;
                            double t = -c(j * 3 + 1) - f(2) * a(j * 3 + 1, j * 3 + 2) - f(3) * a(j * 3 + 1, j * 3 + 3);
                            if (abs(t - f(1)) < 1e-4)
                                break;
                            f(1) = t;
                            double c1 = -c(j * 3 + 2) - f(1) * a(j * 3 + 2, j * 3 + 1);
                            double c2 = -c(j * 3 + 3) - f(1) * a(j * 3 + 3, j * 3 + 1);
                            f(3) = (a(j * 3 + 2, j * 3 + 2) * c2 - a(j * 3 + 3, j * 3 + 2) * c1) / (a(j * 3 + 2, j * 3 + 2) * a(j * 3 + 3, j * 3 + 3) - a(j * 3 + 2, j * 3 + 3) * a(j * 3 + 3, j * 3 + 2));
                            f(2) = (c1 - f(3) * a(j * 3 + 3, j * 3 + 2)) / a(j * 3 + 2, j * 3 + 2);
                            len = sqrt(f(2) * f(2) + f(3) * f(3)) / (f(1) * contacts[j].mu);
                        }
                    }
                }
            }
        
            for (int k = 1; k <= 3; ++k) {
                double delta_t = f(k) - X(k + j * 3);
                for (int l = 0; l < contacts.size(); ++l) {
                    if (l != j) {
                        for (int m = l * 3 + 1; m <= (l + 1) * 3; ++m) {
                            c(m) += a(m,k + j * 3) * delta_t;
                        }
                    }
                }
                X(k + j * 3) += delta_t;
                if (abs(delta_t) > 1e-4) {
                    flag = true;
                }
            }
        }
        if (!flag)
            break;
    }
    

    for (int i = 0; i < contacts.size(); ++i) {
        Contact &ci = contacts[i];
        Vector3d force = ci.n * X(i * 3 + 1) + ci.t1 * X(i * 3 + 2) + ci.t2 * X(i * 3 + 3);
        ci.a->ExcertForce(force);
        ci.b->ExcertForce(-force);
        ci.a->ExcertMoment(force, ci.p);
        ci.b->ExcertMoment(-force, ci.p);
    }
}