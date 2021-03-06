#include "Rcpp.h"
#include "CoefGen.h"
#include "PhiGen.h"
#include "UGen.h"
#include "WGen.h"
#include "XGen.h"
#include "XiGen.h"

#define DSP_BAYES_N_INTERRUPT_CHECK 1000

int* d2s;
bool g_record_status = false;

// Rcpp::List collect_output(const CoefGen& regr_coefs,
// 			  const XiGen& xi,
// 			  const PhiGen& phi);

// w_day_blocks          used when sampling W
// w_to_days_idx         categorical gamma: a_tilde
// w_cyc_to_subj_idx     used when sampling xi (first term)
// fw_len                how much memory to set aside when sampling W in a cycle
// subj_day_block        used when sampling xi (second term)
// gamma_specs           gamma hyperparameters
// phi_specs             phi hyperparameters




// [[Rcpp::export]]
Rcpp::List dsp_(Rcpp::NumericMatrix u_rcpp,
		Rcpp::IntegerVector x_rcpp,
		Rcpp::List          w_day_blocks,
		Rcpp::IntegerVector w_to_days_idx,
		Rcpp::IntegerVector w_cyc_to_subj_idx,
		Rcpp::List          subj_day_blocks,
		Rcpp::IntegerVector day_to_subj_idx,
		Rcpp::List          gamma_specs,
		Rcpp::NumericVector phi_specs,
		Rcpp::IntegerVector x_miss,
		Rcpp::IntegerVector sex_miss_to_w,
		Rcpp::NumericVector utau_rcpp,
		Rcpp::List          tau_coefs,
		Rcpp::List          u_miss_info,
		Rcpp::IntegerVector u_miss_type,
		Rcpp::IntegerVector u_preg_map,
		Rcpp::IntegerVector u_sex_map,
		int fw_len,
		int n_burn,
		int n_samp) {

    // initialize global variable in case the value was set to true elsewhere
    g_record_status = false;
    bool is_verbose = true;
    d2s = day_to_subj_idx.begin();

    // create data objects
    WGen W(w_day_blocks, w_to_days_idx, w_cyc_to_subj_idx, fw_len);
    XiGen xi(subj_day_blocks, n_samp, is_verbose);
    CoefGen coefs(u_rcpp, gamma_specs, n_samp);
    PhiGen phi(phi_specs, n_samp, is_verbose);  // TODO: need a variable for keeping samples
    UProdBeta ubeta(u_rcpp.nrow());
    XGen X(x_rcpp, x_miss_cyc, x_miss_day, tau_coefs["cohort_sex_prob"], tau_coefs["sex_coef"]);
    UProdTau utau(utau_rcpp, tau_coefs);
    UGen U(u_rcpp, u_miss_info, u_miss_type, u_preg_map, u_sex_map, is_verbose);

    // begin sampler loop
    for (int s = 0; s < n_samp; s++) {

    	// update the latent day-specific pregnancy variables W
    	W.sample(xi, ubeta, X);

    	// update the woman-specific fecundability multipliers xi
    	xi.sample(W, phi, ubeta, X);

    	// update the regression coefficients gamma and psi, and update the
    	// resulting values of the `U * beta`
    	coefs.sample(W, xi, ubeta, X.vals());
	ubeta.update_exp();  // <--- TODO: let's put this inside sample()

    	// update phi, the variance parameter for xi
    	phi.sample(xi);

	// update missing values for the intercourse variables X
	X.sample(W, xi, ubeta, utau);

	// // update missing values for the covariate data U
	U.sample(W, xi, coefs, X, ubeta, utau);

	// case: burn-in phase is over so record samples.  Note that this occurs
	// after the samples in this scan have been taken; this is because
	// `g_record_status` has the effect of informing the various classes to
	// not overwrite previous data.
	if (s == 0) g_record_status = true;

	// check for user interrupt every `DSP_BAYES_N_INTER_CHECK` iterations
	if ((s % DSP_BAYES_N_INTERRUPT_CHECK) == 0) Rcpp::checkUserInterrupt();
    }

    return Rcpp::List::create(Rcpp::Named("coefs") = coefs.m_vals_rcpp,
			      Rcpp::Named("xi")    = xi.m_vals_rcpp,
			      Rcpp::Named("phi")   = phi.m_vals_rcpp,
			      Rcpp::Named("ugen")  = U.realized_samples());
}
