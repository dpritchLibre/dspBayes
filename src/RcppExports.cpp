// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <Rcpp.h>

using namespace Rcpp;

// dsp_
Rcpp::List dsp_(Rcpp::NumericMatrix U, Rcpp::IntegerVector X_rcpp, Rcpp::List w_day_blocks, Rcpp::IntegerVector w_to_days_idx, Rcpp::IntegerVector w_cyc_to_subj_idx, Rcpp::List subj_day_blocks, Rcpp::IntegerVector day_to_subj_idx, Rcpp::List gamma_specs, Rcpp::NumericVector phi_specs, Rcpp::List x_miss_cyc, Rcpp::List x_miss_day, Rcpp::NumericVector utau_rcpp, Rcpp::List tau_coefs, int fw_len, int n_burn, int n_samp);
RcppExport SEXP _dspBayes_dsp_(SEXP USEXP, SEXP X_rcppSEXP, SEXP w_day_blocksSEXP, SEXP w_to_days_idxSEXP, SEXP w_cyc_to_subj_idxSEXP, SEXP subj_day_blocksSEXP, SEXP day_to_subj_idxSEXP, SEXP gamma_specsSEXP, SEXP phi_specsSEXP, SEXP x_miss_cycSEXP, SEXP x_miss_daySEXP, SEXP utau_rcppSEXP, SEXP tau_coefsSEXP, SEXP fw_lenSEXP, SEXP n_burnSEXP, SEXP n_sampSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< Rcpp::NumericMatrix >::type U(USEXP);
    Rcpp::traits::input_parameter< Rcpp::IntegerVector >::type X_rcpp(X_rcppSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type w_day_blocks(w_day_blocksSEXP);
    Rcpp::traits::input_parameter< Rcpp::IntegerVector >::type w_to_days_idx(w_to_days_idxSEXP);
    Rcpp::traits::input_parameter< Rcpp::IntegerVector >::type w_cyc_to_subj_idx(w_cyc_to_subj_idxSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type subj_day_blocks(subj_day_blocksSEXP);
    Rcpp::traits::input_parameter< Rcpp::IntegerVector >::type day_to_subj_idx(day_to_subj_idxSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type gamma_specs(gamma_specsSEXP);
    Rcpp::traits::input_parameter< Rcpp::NumericVector >::type phi_specs(phi_specsSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type x_miss_cyc(x_miss_cycSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type x_miss_day(x_miss_daySEXP);
    Rcpp::traits::input_parameter< Rcpp::NumericVector >::type utau_rcpp(utau_rcppSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type tau_coefs(tau_coefsSEXP);
    Rcpp::traits::input_parameter< int >::type fw_len(fw_lenSEXP);
    Rcpp::traits::input_parameter< int >::type n_burn(n_burnSEXP);
    Rcpp::traits::input_parameter< int >::type n_samp(n_sampSEXP);
    rcpp_result_gen = Rcpp::wrap(dsp_(U, X_rcpp, w_day_blocks, w_to_days_idx, w_cyc_to_subj_idx, subj_day_blocks, day_to_subj_idx, gamma_specs, phi_specs, x_miss_cyc, x_miss_day, utau_rcpp, tau_coefs, fw_len, n_burn, n_samp));
    return rcpp_result_gen;
END_RCPP
}
// utest_cpp_
int utest_cpp_(Rcpp::NumericMatrix U, Rcpp::IntegerVector X_rcpp, Rcpp::List w_day_blocks, Rcpp::IntegerVector w_to_days_idx, Rcpp::IntegerVector w_cyc_to_subj_idx, Rcpp::List subj_day_blocks, Rcpp::IntegerVector day_to_subj_idx, Rcpp::List gamma_specs, Rcpp::NumericVector phi_specs, Rcpp::List x_miss_cyc, Rcpp::List x_miss_day, Rcpp::NumericVector utau_rcpp, Rcpp::List tau_coefs, int fw_len, int n_burn, int n_samp, Rcpp::List test_data);
RcppExport SEXP _dspBayes_utest_cpp_(SEXP USEXP, SEXP X_rcppSEXP, SEXP w_day_blocksSEXP, SEXP w_to_days_idxSEXP, SEXP w_cyc_to_subj_idxSEXP, SEXP subj_day_blocksSEXP, SEXP day_to_subj_idxSEXP, SEXP gamma_specsSEXP, SEXP phi_specsSEXP, SEXP x_miss_cycSEXP, SEXP x_miss_daySEXP, SEXP utau_rcppSEXP, SEXP tau_coefsSEXP, SEXP fw_lenSEXP, SEXP n_burnSEXP, SEXP n_sampSEXP, SEXP test_dataSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< Rcpp::NumericMatrix >::type U(USEXP);
    Rcpp::traits::input_parameter< Rcpp::IntegerVector >::type X_rcpp(X_rcppSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type w_day_blocks(w_day_blocksSEXP);
    Rcpp::traits::input_parameter< Rcpp::IntegerVector >::type w_to_days_idx(w_to_days_idxSEXP);
    Rcpp::traits::input_parameter< Rcpp::IntegerVector >::type w_cyc_to_subj_idx(w_cyc_to_subj_idxSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type subj_day_blocks(subj_day_blocksSEXP);
    Rcpp::traits::input_parameter< Rcpp::IntegerVector >::type day_to_subj_idx(day_to_subj_idxSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type gamma_specs(gamma_specsSEXP);
    Rcpp::traits::input_parameter< Rcpp::NumericVector >::type phi_specs(phi_specsSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type x_miss_cyc(x_miss_cycSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type x_miss_day(x_miss_daySEXP);
    Rcpp::traits::input_parameter< Rcpp::NumericVector >::type utau_rcpp(utau_rcppSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type tau_coefs(tau_coefsSEXP);
    Rcpp::traits::input_parameter< int >::type fw_len(fw_lenSEXP);
    Rcpp::traits::input_parameter< int >::type n_burn(n_burnSEXP);
    Rcpp::traits::input_parameter< int >::type n_samp(n_sampSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type test_data(test_dataSEXP);
    rcpp_result_gen = Rcpp::wrap(utest_cpp_(U, X_rcpp, w_day_blocks, w_to_days_idx, w_cyc_to_subj_idx, subj_day_blocks, day_to_subj_idx, gamma_specs, phi_specs, x_miss_cyc, x_miss_day, utau_rcpp, tau_coefs, fw_len, n_burn, n_samp, test_data));
    return rcpp_result_gen;
END_RCPP
}

static const R_CallMethodDef CallEntries[] = {
    {"_dspBayes_dsp_", (DL_FUNC) &_dspBayes_dsp_, 16},
    {"_dspBayes_utest_cpp_", (DL_FUNC) &_dspBayes_utest_cpp_, 17},
    {NULL, NULL, 0}
};

RcppExport void R_init_dspBayes(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
