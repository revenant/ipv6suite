#ifndef _hodec_INFERENCE_ENGINE_HPP
#define _hodec_INFERENCE_ENGINE_HPP

#include "xfuzzy.h"

//+++++++++++++++++++++++++++++++++++++//
//  MembershipFunction MF_hodec_xfl_triangle  //
//+++++++++++++++++++++++++++++++++++++//

class MF_hodec_xfl_triangle: public ParamMembershipFunction {
private:
 double a;
 double b;
 double c;

public:
 MF_hodec_xfl_triangle() {};
 virtual ~MF_hodec_xfl_triangle() {};
 MF_hodec_xfl_triangle(double min,double max,double step,double *param, int length);
 MF_hodec_xfl_triangle*dup();
 virtual double param(int _i);
 virtual double compute_eq(double x);
 virtual double compute_greq(double x);
 virtual double compute_smeq(double x);
 virtual double center();
 virtual double basis();
};

//+++++++++++++++++++++++++++++++++++++//
//  Operatorset OP_hodec__default_ //
//+++++++++++++++++++++++++++++++++++++//

class OP_hodec__default_: public Operatorset {
public:
 virtual ~OP_hodec__default_() {};
 virtual double and2(double a, double b);
 virtual double or2(double a, double b);
 virtual double also(double a, double b);
 virtual double imp(double a, double b);
 virtual double not2(double a);
 virtual double very(double a);
 virtual double moreorless(double a);
 virtual double slightly(double a);
 virtual double defuz(OutputMembershipFunction &mf);
};

//+++++++++++++++++++++++++++++++++++++//
//  Type TP_hodec_bw_req //
//+++++++++++++++++++++++++++++++++++++//

class TP_hodec_bw_req {
private:
 double min;
 double max;
 double step;
public:
 MF_hodec_xfl_triangle low;
 MF_hodec_xfl_triangle med;
 MF_hodec_xfl_triangle high;
 TP_hodec_bw_req();
};

//+++++++++++++++++++++++++++++++++++++//
//  Type TP_hodec_avail_bw //
//+++++++++++++++++++++++++++++++++++++//

class TP_hodec_avail_bw {
private:
 double min;
 double max;
 double step;
public:
 MF_hodec_xfl_triangle low;
 MF_hodec_xfl_triangle med;
 MF_hodec_xfl_triangle high;
 TP_hodec_avail_bw();
};

//+++++++++++++++++++++++++++++++++++++//
//  Type TP_hodec_ss //
//+++++++++++++++++++++++++++++++++++++//

class TP_hodec_ss {
private:
 double min;
 double max;
 double step;
public:
 MF_hodec_xfl_triangle low;
 MF_hodec_xfl_triangle high;
 TP_hodec_ss();
};

//+++++++++++++++++++++++++++++++++++++//
//  Type TP_hodec_decision //
//+++++++++++++++++++++++++++++++++++++//

class TP_hodec_decision {
private:
 double min;
 double max;
 double step;
public:
 MF_hodec_xfl_triangle no;
 MF_hodec_xfl_triangle p_no;
 MF_hodec_xfl_triangle p_yes;
 MF_hodec_xfl_triangle yes;
 TP_hodec_decision();
};

//+++++++++++++++++++++++++++++++++++++//
//  Fuzzy Inference Engine hodec  //
//+++++++++++++++++++++++++++++++++++++//

class hodec: public FuzzyInferenceEngine {
public:
 hodec() {};
 virtual ~hodec() {};
 virtual double* crispInference(double* input);
 virtual double* crispInference(MembershipFunction* &input);
 virtual MembershipFunction** fuzzyInference(double* input);
 virtual MembershipFunction** fuzzyInference(MembershipFunction* &input);
 void inference( double _i_ms_bw_req, double _i_ap_ss, double _i_ap_avail_bw, double *_o_ho_decision );
private:
 void RL_ho_decision(MembershipFunction &ms_bw_req, MembershipFunction &ap_ss, MembershipFunction &ap_avail_bw, MembershipFunction ** _o_ho_value);
};

#endif /* _hodec_INFERENCE_ENGINE_HPP */

