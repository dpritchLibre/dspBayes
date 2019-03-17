#include <cmath>
#include "Rcpp.h"

#include "CoefGen.h"
#include "GammaFWDay.h"
#include "FWPriors.h"
#include "ProposalFcns.h"


Mu::Mu(int n_samp, bool record_status, double proposal_dispersion) :
    MHCont(n_samp, record_status, proposal_dispersion),
    m_alpha_0_minus_1 {1.0},
    m_beta_0          {2.0 / 0.44},
    m_mu_val          {0.44},
    m_log_mu_val      {std::log(0.44)}
{
    // sync to the initial value
    *m_vals = m_mu_val;
}




// update the value of `m_mu_val` using a Metropolis step
void Mu::sample(const CoefGen& coefs,
                const MDay& mday,
                const Nu& nu,
                const Delta& delta) {

    // sample the proposal value for Metropolis step.  TODO: generalize
    const double proposal_val = ProposalFcns::abs_unif(*m_vals, m_prp_disp);
    const double log_proposal_val = std::log(proposal_val);

    // calculate `log(r)` where `r` is the acceptance ratio for the Metropolis
    // step
    double log_r = calc_log_r(coefs, mday, nu, delta, proposal_val, log_proposal_val);

    // sample the updated value by either accepting the proposal value or by
    // keeping the current value
    double new_val = update(log_r, proposal_val);
    if (new_val != m_mu_val) {
        m_mu_val = new_val;
        m_log_mu_val = std::log(m_mu_val);
    }

    // save the value of the new sample.  If we are recording samples then
    // increment `m_vals` so that we don't overwrite the previous sample.
    if (m_record_status && g_record_status) {
        ++m_vals;
    }
    *m_vals = new_val;
}




// calculate the value of `log(r)`, where `r` is the value such that the
// proposal value is accepted with probability `min(1,r)`
double Mu::calc_log_r(const CoefGen& coefs,
                      const MDay& mday,
                      const Nu& nu,
                      const Delta& delta,
                      double proposal_val,
                      double log_proposal_val) const {

    return calc_log_lik_gamma_term(coefs, mday, nu, delta, proposal_val, log_proposal_val)
        + calc_log_lik_mu_term(proposal_val, log_proposal_val);
}




// calculate
//
//         p(gamma_1, ..., gamma_K | m, proposal_mu, nu, delta)
//     log ----------------------------------------------------
//         p(gamma_1, ..., gamma_K | m, current_mu, nu, delta)
//
//                           p(gamma_k | m, proposal_mu, nu, delta)
//         = sum_{k=1}^K log --------------------------------------.
//                           p(gamma_k | m, current_mu, nu, delta)
//
// We note that the k-th term of the sum simplifies to
//
//     -nu * (log(proposal_mu) - log(current_mu))
//
//           nu * gamma_k
//         - ------------- ((1 / proposal_mu) - (1 / current_mu))
//           delta^{|k-m|}
//
// Additionally, the `-nu` term can be factored out of each term in the sum for
// efficiency.

double Mu::calc_log_lik_gamma_term(const CoefGen& coefs,
                                   const MDay& mday,
                                   const Nu& nu,
                                   const Delta& delta,
                                   double proposal_val,
                                   double log_proposal_val) const {

    // extract priors for convenience
    const double mday_val  = mday.val();
    const double nu_val    = nu.val();
    const double delta_val = delta.val();

    // used to collect the sum log-likelihood ratio of the individual gamma terms
    double sum_term = 0;
    int K = 0;  // TODO: make available in CoefGen.h

    // each iteration conditionally calculates the log-likelihood ratio portion
    // in the sum of the k-th gamma term and adds it to `sum_term`
    for (int k = 0; k < coefs.m_n_gamma; ++k) {

        // only consider the gamma terms that are part of the fertile window
        // TODO: can we make an interface for gamma params?
        if (coefs.m_gamma[k]->is_fw_day()) {

            // current FW day index and coefficient value
            int curr_day_idx    = dynamic_cast<GammaFWDay*>(coefs.m_gamma[k])->m_day_idx;
            double curr_gam_val = coefs.m_gamma[k]->m_gam_val;

            // calculate `delta^{|k-m|}`
            double day_dist  = abs(curr_day_idx - mday_val);
            double delta_pow = pow(delta_val, day_dist);

            // add `gamma_k / delta^{|k-m|}` to the running total
            sum_term += curr_gam_val / delta_pow;
            K += 1;
        }
    }

    //

    // calculate the terms after including the non-indexed portions
    double term1 = K * (log_proposal_val - m_log_mu_val);
    double term2 = ((1 / proposal_val) - (1 / m_mu_val)) * sum_term;

    return -nu_val * (term1 + term2);
}




// calculate
//
//         p(proposal_mu | a, b)
//     log ---------------------
//         p(current_mu | a, b)
//
//         = (a - 1) * (log(proposal_mu) - log(current_mu))
//
//             - b * (proposal_mu - current_mu)

double Mu::calc_log_lik_mu_term(double proposal_val, double log_proposal_val) const {

    double term1 = m_alpha_0_minus_1 * (log_proposal_val - m_log_mu_val);
    double term2 = m_beta_0 * (proposal_val - m_mu_val);

    return term1 - term2;
}
