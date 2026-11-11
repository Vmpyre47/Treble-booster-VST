// Copyright (c) 2026 c.d.r.c plugins. Licensed under GNU AGPLv3.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Original positive-ground PNP circuit, solved in polarity-normalised NPN
// coordinates. Negate both audio ports to recover the original PNP polarity.
// Trapezoidal capacitor companions + Ebers-Moll, with analytic Jacobian.
// Model parameters represent nominal devices, not a measured specimen.
namespace rm {
struct Transistor {
    double is, betaF, betaR, leakage;
    static Transistor germanium() { return {8.0e-8, 90.0, 2.0, 15e-6}; }
    static Transistor silicon() { return {6.7e-15, 160.0, 3.0, 0.0}; }
};

class Circuit {
public:
    void prepare(double rate, Transistor device, double initialBoost=1.0) {
        fs = rate; q = device; failed = 0; maxResidual = 0;
        v = {1.0, device.is > 1e-10 ? 0.8 : 0.4, 7.0};
        solve(0, 1, true);
        const double t=taper(initialBoost);
        inputCap = v[0]; emitterCap = v[1]; outputCap = 9*(1-t)+t*v[2];
    }
    double tick(double audio, double boost) {
        const double t = taper(boost);
        const double input = -audio;
        solve(input, t, false);
        const double gw = outputConductance();
        const double z = 10000.0 * t * (1.0-t);
        const double w = (9.0*(1.0-t) + t*v[2] + z*gw*outputCap) / (1.0+z*gw);
        const double out = (2*10e-9*fs)/(2*10e-9*fs + 1e-6) * (w-outputCap);
        outputCap = 2*(w-out)-outputCap;
        inputCap = 2*(v[0]-input)-inputCap;
        emitterCap = 2*v[1]-emitterCap;
        return -out;
    }
    const std::array<double,3>& bias() const { return v; }
    uint64_t failures() const { return failed; }
    double residual() const { return maxResidual; }
private:
    static double taper(double boost) {
        return (std::exp(4.605170185988091*std::clamp(boost,0.0,1.0))-1.0)/99.0;
    }
    static constexpr double vt = 0.02585;
    double fs = 192000, inputCap = 0, emitterCap = 0, outputCap = 0;
    Transistor q = Transistor::germanium();
    std::array<double,3> v{};
    uint64_t failed = 0;
    double maxResidual = 0;
    double outputConductance() const {
        const double g = 2*10e-9*fs;
        return g*1e-6/(g+1e-6); // C2 loaded by the documented 1 Mohm amp input
    }
    static double expSafe(double x) {
        if (x > 40.0) return std::exp(40.0)*(1.0+x-40.0);
        return std::exp(std::max(-80.0,x));
    }
    static double expDerivative(double x) {
        return std::exp(std::clamp(x,-80.0,40.0));
    }
    void equations(const std::array<double,3>& x, double input, double t, bool dc,
                   double (&f)[3], double (&j)[3][3]) const {
        const double af=q.betaF/(q.betaF+1), ar=q.betaR/(q.betaR+1);
        const double a=(x[0]-x[1])/vt, b=(x[0]-x[2])/vt;
        const double ef=q.is/af*(expSafe(a)-1), er=q.is/ar*(expSafe(b)-1);
        const double df=q.is/af/vt*expDerivative(a), dr=q.is/ar/vt*expDerivative(b);
        const double ib=(1-af)*ef+(1-ar)*er;
        const double ie=-ef+ar*er-q.leakage;
        const double ic=af*ef-er+q.leakage;
        const double gi=dc?0:2*5e-9*fs, ge=dc?0:2*47e-6*fs;
        const double gw=dc?0:outputConductance(), z=10000*t*(1-t), d=1+z*gw;
        const double gc=1e-4+t*t*gw/d;
        const double rhs=9e-4+t*gw*(outputCap-9*(1-t))/d;
        f[0]=x[0]/68000+(x[0]-9)/470000+gi*(x[0]-input-inputCap)+ib;
        f[1]=x[1]/3900+ge*(x[1]-emitterCap)+ie;
        f[2]=gc*x[2]-rhs+ic;
        j[0][0]=1.0/68000+1.0/470000+gi+(1-af)*df+(1-ar)*dr;
        j[0][1]=-(1-af)*df; j[0][2]=-(1-ar)*dr;
        j[1][0]=-df+ar*dr; j[1][1]=1.0/3900+ge+df; j[1][2]=-ar*dr;
        j[2][0]=af*df-dr; j[2][1]=-af*df; j[2][2]=gc+dr;
    }
    void solve(double input, double t, bool dc) {
        auto x=v;
        double err=0;
        // Bounded work on the audio thread; pivoted 3x3 Newton solve.
        for(int iter=0; iter<(dc?120:80); ++iter) {
            double f[3],j[3][3]; equations(x,input,t,dc,f,j);
            err=std::max({std::abs(f[0]),std::abs(f[1]),std::abs(f[2])});
            if(err < 1e-13) { v=x; return; }
            double m[3][4];
            for(int r=0;r<3;++r) { for(int c=0;c<3;++c) m[r][c]=j[r][c]; m[r][3]=-f[r]; }
            for(int k=0;k<3;++k) {
                int p=k; for(int r=k+1;r<3;++r) if(std::abs(m[r][k])>std::abs(m[p][k])) p=r;
                if(p!=k) for(int c=k;c<4;++c) std::swap(m[p][c],m[k][c]);
                for(int r=k+1;r<3;++r) { const double a=m[r][k]/m[k][k]; for(int c=k;c<4;++c) m[r][c]-=a*m[k][c]; }
            }
            double dx[3]{};
            for(int r=2;r>=0;--r) { double a=m[r][3]; for(int c=r+1;c<3;++c) a-=m[r][c]*dx[c]; dx[r]=a/m[r][r]; }
            // Limit junction changes, while allowing the collector to swing.
            double damp=1.0;
            const auto limitJunction=[&damp](double old,double delta) {
                if(delta>0 && old+delta>0.10)
                    damp=std::min(damp,(old<0?0.10-old:0.10)/delta);
                else if(delta<0 && old>0.10)
                    damp=std::min(damp,(old+0.10)/(-delta));
            };
            limitJunction(x[0]-x[1],dx[0]-dx[1]);
            limitJunction(x[0]-x[2],dx[0]-dx[2]);
            // Residual backtracking prevents a collector step from jumping
            // across the exponential base-collector saturation knee.
            std::array<double,3> candidate{};
            for(int line=0;line<14;++line) {
                for(int r=0;r<3;++r) candidate[r]=x[r]+damp*dx[r];
                double cf[3],cj[3][3]; equations(candidate,input,t,dc,cf,cj);
                const double next=std::max({std::abs(cf[0]),std::abs(cf[1]),std::abs(cf[2])});
                if(next<err || next<1e-13) break;
                damp*=0.5;
            }
            x=candidate;
        }
        double f[3],j[3][3]; equations(x,input,t,dc,f,j);
        err=std::max({std::abs(f[0]),std::abs(f[1]),std::abs(f[2])});
        maxResidual=std::max(maxResidual,err);
        if(err>1e-8) ++failed;
        if(std::isfinite(x[0])&&std::isfinite(x[1])&&std::isfinite(x[2])) v=x;
    }
};
}
