#ifndef _hodec_INFERENCE_ENGINE_
#define _hodec_INFERENCE_ENGINE_

#include <stdio.h>
#include <math.h>
#include "xfuzzy.h"
#include "hodec.h"

//+++++++++++++++++++++++++++++++++++++//
//  MembershipFunction MF_hodec_xfl_triangle  //
//+++++++++++++++++++++++++++++++++++++//

MF_hodec_xfl_triangle::MF_hodec_xfl_triangle(double min,double max,double step,double *param, int length) :
ParamMembershipFunction(min,max,step) {
 this->name = "MF_hodec_xfl_triangle";
 this->a = param[0];
 this->b = param[1];
 this->c = param[2];
}

MF_hodec_xfl_triangle * MF_hodec_xfl_triangle::dup() {
 double param[3] = {a,b,c};
 return new MF_hodec_xfl_triangle(min,max,step,param,3);
}

double MF_hodec_xfl_triangle::param(int _i) {
 switch(_i) {
  case 0: return a;
  case 1: return b;
  case 2: return c;
  default: return 0;
 }
}

double MF_hodec_xfl_triangle::compute_eq(double x) {
  return (a<x && x<=b? (x-a)/(b-a) : (b<x && x<c? (c-x)/(c-b) : 0)); 
}

double MF_hodec_xfl_triangle::compute_greq(double x) {
  return (x<a? 0 : (x>b? 1 : (x-a)/(b-a) )); 
}

double MF_hodec_xfl_triangle::compute_smeq(double x) {
  return (x<b? 1 : (x>c? 0 : (c-x)/(c-b) )); 
}

double MF_hodec_xfl_triangle::center() {
  return b; 
}

double MF_hodec_xfl_triangle::basis() {
  return (c-a); 
}

//+++++++++++++++++++++++++++++++++++++//
//  Operatorset OP_hodec__default_ //
//+++++++++++++++++++++++++++++++++++++//

double OP_hodec__default_::and2(double a, double b) {
  return (a<b? a : b); 
}
double OP_hodec__default_::or2(double a, double b) {
  return (a>b? a : b); 
}
double OP_hodec__default_::also(double a, double b) {
  return (a>b? a : b); 
}
double OP_hodec__default_::imp(double a, double b) {
  return (a<b? a : b); 
}
double OP_hodec__default_::not2(double a) {
  return 1-a; 
}

double OP_hodec__default_::very(double a) {
 double w = 2.0;
  return pow(a,w); 
}

double OP_hodec__default_::moreorless(double a) {
 double w = 0.5;
  return pow(a,w); 
}

double OP_hodec__default_::slightly(double a) {
  return 4*a*(1-a); 
}

double OP_hodec__default_::defuz(OutputMembershipFunction &mf) {
 double min = mf.min();
 double max = mf.max();
 double step = mf.step();
   double num=0, denom=0;
   for(double x=min; x<=max; x+=step) {
    double m = mf.compute(x);
    num += x*m;
    denom += m;
   }
   if(denom==0) return (min+max)/2;
   return num/denom;
}

//+++++++++++++++++++++++++++++++++++++//
//  Type TP_hodec_bw_req //
//+++++++++++++++++++++++++++++++++++++//

TP_hodec_bw_req::TP_hodec_bw_req() {
 min = 0.0;
 max = 1.0;
 step = 0.00392156862745098;
 double _p_low[3] = { -0.5,0.0,0.5 };
 double _p_med[3] = { 0.0,0.5,1.0 };
 double _p_high[3] = { 0.5,1.0,1.5 };
 low = MF_hodec_xfl_triangle(min,max,step,_p_low,3);
 med = MF_hodec_xfl_triangle(min,max,step,_p_med,3);
 high = MF_hodec_xfl_triangle(min,max,step,_p_high,3);
}

//+++++++++++++++++++++++++++++++++++++//
//  Type TP_hodec_avail_bw //
//+++++++++++++++++++++++++++++++++++++//

TP_hodec_avail_bw::TP_hodec_avail_bw() {
 min = 0.0;
 max = 1.0;
 step = 0.00392156862745098;
 double _p_low[3] = { -0.5,0.0,0.5 };
 double _p_med[3] = { 0.0,0.5,1.0 };
 double _p_high[3] = { 0.5,1.0,1.5 };
 low = MF_hodec_xfl_triangle(min,max,step,_p_low,3);
 med = MF_hodec_xfl_triangle(min,max,step,_p_med,3);
 high = MF_hodec_xfl_triangle(min,max,step,_p_high,3);
}

//+++++++++++++++++++++++++++++++++++++//
//  Type TP_hodec_ss //
//+++++++++++++++++++++++++++++++++++++//

TP_hodec_ss::TP_hodec_ss() {
 min = 0.0;
 max = 1.0;
 step = 0.00392156862745098;
 double _p_low[3] = { -1.0,0.0,1.0 };
 double _p_high[3] = { 0.0,1.0,2.0 };
 low = MF_hodec_xfl_triangle(min,max,step,_p_low,3);
 high = MF_hodec_xfl_triangle(min,max,step,_p_high,3);
}

//+++++++++++++++++++++++++++++++++++++//
//  Type TP_hodec_decision //
//+++++++++++++++++++++++++++++++++++++//

TP_hodec_decision::TP_hodec_decision() {
 min = 0.0;
 max = 1.0;
 step = 0.00392156862745098;
 double _p_no[3] = { -0.3333333333333333,0.0,0.3333333333333333 };
 double _p_p_no[3] = { 0.0,0.3333333333333333,0.6666666666666666 };
 double _p_p_yes[3] = { 0.3333333333333333,0.6666666666666666,1.0 };
 double _p_yes[3] = { 0.6666666666666666,1.0,1.3333333333333333 };
 no = MF_hodec_xfl_triangle(min,max,step,_p_no,3);
 p_no = MF_hodec_xfl_triangle(min,max,step,_p_p_no,3);
 p_yes = MF_hodec_xfl_triangle(min,max,step,_p_p_yes,3);
 yes = MF_hodec_xfl_triangle(min,max,step,_p_yes,3);
}

//+++++++++++++++++++++++++++++++++++++//
//  Rulebase RL_ho_decision //
//+++++++++++++++++++++++++++++++++++++//

void hodec::RL_ho_decision(MembershipFunction &ms_bw_req, MembershipFunction &ap_ss, MembershipFunction &ap_avail_bw, MembershipFunction ** _o_ho_value) {
 OP_hodec__default_ _op;
 double _rl;
 int _i_ho_value=0;
 double *_input = new double[3];
 _input[0] = ms_bw_req.getValue();
 _input[1] = ap_ss.getValue();
 _input[2] = ap_avail_bw.getValue();
 OutputMembershipFunction *ho_value = new OutputMembershipFunction(new OP_hodec__default_(),17,3,_input);
 TP_hodec_bw_req _t_ms_bw_req;
 TP_hodec_ss _t_ap_ss;
 TP_hodec_avail_bw _t_ap_avail_bw;
 TP_hodec_decision _t_ho_value;
 _rl = _op.and2(_t_ms_bw_req.low.isEqual(ms_bw_req),_op.and2(_t_ap_ss.low.isEqual(ap_ss),_t_ap_avail_bw.low.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.p_yes.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.low.isEqual(ms_bw_req),_op.and2(_t_ap_ss.high.isEqual(ap_ss),_t_ap_avail_bw.low.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.yes.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.low.isEqual(ms_bw_req),_op.and2(_t_ap_ss.low.isEqual(ap_ss),_t_ap_avail_bw.med.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.p_no.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.low.isEqual(ms_bw_req),_op.and2(_t_ap_ss.high.isEqual(ap_ss),_t_ap_avail_bw.med.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.p_yes.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.low.isEqual(ms_bw_req),_op.and2(_t_ap_ss.high.isEqual(ap_ss),_t_ap_avail_bw.high.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.p_no.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.med.isEqual(ms_bw_req),_op.and2(_t_ap_ss.low.isEqual(ap_ss),_t_ap_avail_bw.low.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.no.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.med.isEqual(ms_bw_req),_op.and2(_t_ap_ss.low.isEqual(ap_ss),_t_ap_avail_bw.med.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.p_yes.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.med.isEqual(ms_bw_req),_op.and2(_t_ap_ss.high.isEqual(ap_ss),_t_ap_avail_bw.med.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.yes.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.med.isEqual(ms_bw_req),_op.and2(_t_ap_ss.low.isEqual(ap_ss),_t_ap_avail_bw.high.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.p_no.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.med.isEqual(ms_bw_req),_op.and2(_t_ap_ss.high.isEqual(ap_ss),_t_ap_avail_bw.high.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.p_yes.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.high.isEqual(ms_bw_req),_t_ap_avail_bw.low.isEqual(ap_avail_bw));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.no.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.high.isEqual(ms_bw_req),_op.and2(_t_ap_ss.low.isEqual(ap_ss),_t_ap_avail_bw.med.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.no.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.high.isEqual(ms_bw_req),_op.and2(_t_ap_ss.high.isEqual(ap_ss),_t_ap_avail_bw.med.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.p_no.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.high.isEqual(ms_bw_req),_op.and2(_t_ap_ss.low.isEqual(ap_ss),_t_ap_avail_bw.high.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.p_yes.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.high.isEqual(ms_bw_req),_op.and2(_t_ap_ss.high.isEqual(ap_ss),_t_ap_avail_bw.high.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.yes.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.low.isEqual(ms_bw_req),_op.and2(_t_ap_ss.low.isEqual(ap_ss),_t_ap_avail_bw.high.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.no.dup()); _i_ho_value++;
 _rl = _op.and2(_t_ms_bw_req.med.isEqual(ms_bw_req),_op.and2(_t_ap_ss.high.isEqual(ap_ss),_t_ap_avail_bw.low.isEqual(ap_avail_bw)));
 (*ho_value).conc[_i_ho_value] = new RuleConclusion(_rl, _t_ho_value.p_no.dup()); _i_ho_value++;
 *_o_ho_value = ho_value;
 delete _input;
}
//+++++++++++++++++++++++++++++++++++++//
//          Inference Engine           //
//+++++++++++++++++++++++++++++++++++++//

double* hodec::crispInference(double *_input) {
 FuzzySingleton ms_bw_req(_input[0]);
 FuzzySingleton ap_ss(_input[1]);
 FuzzySingleton ap_avail_bw(_input[2]);
 MembershipFunction *ho_decision;
 RL_ho_decision(ms_bw_req, ap_ss, ap_avail_bw, &ho_decision);
 double *_output = new double[1];
 if(ho_decision->getType() == MembershipFunction::CRISP) _output[0] = ((FuzzySingleton *) ho_decision)->getValue();
 else _output[0] = ((OutputMembershipFunction *) ho_decision)->defuzzify();
 delete ho_decision;
 return _output;
}

double* hodec::crispInference(MembershipFunction* &_input) {
 MembershipFunction & ms_bw_req = _input[0];
 MembershipFunction & ap_ss = _input[1];
 MembershipFunction & ap_avail_bw = _input[2];
 MembershipFunction *ho_decision;
 RL_ho_decision(ms_bw_req, ap_ss, ap_avail_bw, &ho_decision);
 double *_output = new double[1];
 if(ho_decision->getType() == MembershipFunction::CRISP) _output[0] = ((FuzzySingleton *) ho_decision)->getValue();
 else _output[0] = ((OutputMembershipFunction *) ho_decision)->defuzzify();
 delete ho_decision;
 return _output;
}

MembershipFunction** hodec::fuzzyInference(double *_input) {
 FuzzySingleton ms_bw_req(_input[0]);
 FuzzySingleton ap_ss(_input[1]);
 FuzzySingleton ap_avail_bw(_input[2]);
 MembershipFunction *ho_decision;
 RL_ho_decision(ms_bw_req, ap_ss, ap_avail_bw, &ho_decision);
 MembershipFunction **_output = new MembershipFunction *[1];
 _output[0] = ho_decision;
 return _output;
}

MembershipFunction** hodec::fuzzyInference(MembershipFunction* &_input) {
 MembershipFunction & ms_bw_req = _input[0];
 MembershipFunction & ap_ss = _input[1];
 MembershipFunction & ap_avail_bw = _input[2];
 MembershipFunction *ho_decision;
 RL_ho_decision(ms_bw_req, ap_ss, ap_avail_bw, &ho_decision);
 MembershipFunction **_output = new MembershipFunction *[1];
 _output[0] = ho_decision;
 return _output;
}

void hodec::inference( double _i_ms_bw_req, double _i_ap_ss, double _i_ap_avail_bw, double *_o_ho_decision ) {
 FuzzySingleton ms_bw_req(_i_ms_bw_req);
 FuzzySingleton ap_ss(_i_ap_ss);
 FuzzySingleton ap_avail_bw(_i_ap_avail_bw);
 MembershipFunction *ho_decision;
 RL_ho_decision(ms_bw_req, ap_ss, ap_avail_bw, &ho_decision);
 if(ho_decision->getType() == MembershipFunction::CRISP) (*_o_ho_decision) = ((FuzzySingleton *) ho_decision)->getValue();
 else (*_o_ho_decision) = ((OutputMembershipFunction *) ho_decision)->defuzzify();
 delete ho_decision;
}

#endif /* _hodec_INFERENCE_ENGINE_ */
