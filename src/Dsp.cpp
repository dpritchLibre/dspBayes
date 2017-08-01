#include "Rcpp.h"
#include "CoefGen.h"
#include "PhiGen.h"
#include "WGen.h"
#include "XiGen.h"

#define DSP_BAYES_N_INTERRUPT_CHECK 1000

int* d2s;
bool g_record_status = false;

Rcpp::List collect_output(const CoefGen& regr_coefs,
			  const XiGen& xi,
			  const PhiGen& phi);

// w_day_blocks          used when sampling W
// w_to_days_idx         categorical gamma: a_tilde
// w_cyc_to_subj_idx     used when sampling xi (first term)
// fw_len                how much memory to set aside when sampling W in a cycle
// subj_day_block        used when sampling xi (second term)
// gamma_specs           gamma hyperparameters
// phi_specs             phi hyperparameters




// [[Rcpp::export]]
Rcpp::List dsp_(Rcpp::NumericMatrix U,
		Rcpp::IntegerVector X_rcpp,
		Rcpp::List w_day_blocks,
		Rcpp::IntegerVector w_to_days_idx,
		Rcpp::IntegerVector w_cyc_to_subj_idx,
		Rcpp::List subj_day_blocks,
		Rcpp::IntegerVector day_to_subj_idx,
		Rcpp::List gamma_specs,
		Rcpp::NumericVector phi_specs,
		int fw_len,
		int n_burn,
		int n_samp) {

    // initialize global variable in case the value was set to true elsewhere
    g_record_status = false;

    // create data objects
    WGen W(w_day_blocks, w_to_days_idx, w_cyc_to_subj_idx, fw_len);
    XiGen xi(subj_day_blocks, n_samp, true);
    CoefGen coefs(U, gamma_specs, n_samp);
    PhiGen phi(phi_specs, n_samp, true);  // TODO: need a variable for keeping samples
    UProdBeta u_prod_beta(U.size());
    int* X = X_rcpp.begin();
    d2s = day_to_subj_idx.begin();

    // begin sampler loop
    for (int s = 0; s < n_samp; s++) {

    	// update the latent day-specific pregnancy variables W
    	W.sample(xi, u_prod_beta);

    	// // update the woman-specific fecundability multipliers xi
    	xi.sample(W, phi, u_prod_beta);

    	// update the regression coefficients gamma and psi
    	coefs.sample(W, xi, u_prod_beta, X);
    	// u_prod_beta.update_exp(X);

    	// update phi, the variance parameter for xi
    	phi.sample(xi);

	// case: burn-in phase is over so record samples.  Note that this occurs
	// after the samples in this scan have been taken: this is because this
	// actually informs the classes to not overwrite previous data.
	if (s == 0) g_record_status = true;

	// check for user interrupt every `DSP_BAYES_N_INTER_CHECK` iterations
	if ((s % DSP_BAYES_N_INTERRUPT_CHECK) == 0) Rcpp::checkUserInterrupt();
    }

    return Rcpp::List::create(Rcpp::Named("coefs") = coefs.m_vals_rcpp,
			      Rcpp::Named("xi")    = xi.m_vals_rcpp,
			      Rcpp::Named("phi")   = phi.m_vals_rcpp);
}