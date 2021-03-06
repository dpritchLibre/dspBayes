#include <cmath>
#include "Rcpp.h"
#include "DayBlock.h"
#include "UProdBeta.h"
#include "UProdTau.h"
#include "WGen.h"
#include "XGen.h"
#include "XiGen.h"

#define SEX_YES            1
#define SEX_IMPUTE_SHIFT   2
#define NON_PREG_CYC      -1

using std::exp;




// constructor
XGen::XGen(Rcpp::IntegerVector& X_rcpp,
	   Rcpp::List& miss_cyc,
	   Rcpp::List& miss_day,
	   double cohort_sex_prob,
	   double sex_coef) :
    m_x_rcpp(X_rcpp),
    m_vals(X_rcpp.begin()),
    m_miss_cyc(XMissCyc::list_to_arr(miss_cyc)),
    m_n_miss_cyc(miss_cyc.size()),
    m_miss_day(XGen::XMissDay::list_to_arr(miss_day)),
    m_cohort_sex_prob(cohort_sex_prob),
    m_sex_coef(sex_coef) {
}




// destructor
XGen::~XGen() {
    delete[] m_miss_cyc;
    delete[] m_miss_day;
}




// construct an array of `XMissCyc`s based upon an `Rcpp::List` that provides
// the specifications for each block.  In more detail, an array of `XMissCyc`s is
// created with length given by the length of `block_list`.  Furthermore, it is
// assumed that each element in `block_list` stores integer values that can be
// accessed using names `beg_idx`, `n_days`, `subj_idx`, and `preg`, and which
// are used to initialize the struct's member values of the same name.  The
// return value is a pointer to the beginning of the array.

XGen::XMissCyc* XGen::XMissCyc::list_to_arr(Rcpp::List& block_list) {

    XMissCyc* block_arr = new XMissCyc[block_list.size()];

    // each iteration constructs a new struct based upon the information
    // provided by the t-th element of `block_list`
    for (int t = 0; t < block_list.size(); ++t) {

    	Rcpp::IntegerVector block_list_t = Rcpp::as<Rcpp::IntegerVector>(block_list[t]);
    	block_arr[t] = XMissCyc(block_list_t["beg_idx"],
				block_list_t["n_days"],
				block_list_t["subj_idx"],
				block_list_t["preg_idx"]);
    }

    return block_arr;
}




// construct an array of `XMissDay`s based upon an `Rcpp::List` that provides
// the specifications for each block.  In more detail, an array of `XMissDay`s
// is created with length given by the length of `block_list`.  Furthermore, it
// is assumed that each element in `block_list` stores integer values that can
// be accessed using names `idx` and `prev` and which are used to initialize the
// struct's member values of the same name.  The return value is a pointer to
// the beginning of the array.

XGen::XMissDay* XGen::XMissDay::list_to_arr(Rcpp::List& block_list) {

    XMissDay* block_arr = new XMissDay[block_list.size()];

    // each iteration constructs a new struct based upon the information
    // provided by the t-th element of `block_list`
    for (int t = 0; t < block_list.size(); ++t) {

    	Rcpp::IntegerVector block_list_t = Rcpp::as<Rcpp::IntegerVector>(block_list[t]);
    	block_arr[t] = XMissDay(block_list_t["idx"],
    				block_list_t["prev"]);
    }

    return block_arr;
}




// update the missing values of X
void XGen::sample(const WGen& W,
		  const XiGen& xi,
		  const UProdBeta& ubeta,
		  const UProdTau& utau) {

    const XMissCyc* curr_miss_cyc = m_miss_cyc;
    const XMissCyc* cyc_end = m_miss_cyc + m_n_miss_cyc;
    const int* w_vals = W.vals();

    // each iteration samples the missing intercourse values for the current
    // block of days specified by `curr_miss_cyc`
    for ( ; curr_miss_cyc != cyc_end; ++curr_miss_cyc) {

	sample_cycle(curr_miss_cyc, w_vals, xi, ubeta, utau);
    }
}





// TODO: have to record imputed sex previous using -1 and 2 flags

void XGen::sample_cycle(const XMissCyc* miss_cyc,
			const int* w_vals,
			const XiGen& xi,
			const UProdBeta& ubeta,
			const UProdTau& utau) {

    double prior_prob_yes, posterior_prob_yes;
    int prev_day_sex;

    // value of xi for the subject that `miss_cycl` corresponds to
    const double xi_i = xi.vals()[miss_cyc->subj_idx];

    // either the value of `NON_PREG_CYC` or the index in `W` of the first day
    // in the cycle
    int curr_preg_idx = miss_cyc->preg_idx;

    // if we don't have missing intercourse data for the day before the fertile
    // window, then we sample it using a global probability.  Note that
    // `curr_miss_day` need not be the first day in the fertile window, but if
    // not then the intercourse data for the previous day is known and hence
    // this conditional is still valid.
    if (check_if_prev_sex_miss(miss_cyc->beg_idx)) {

	prev_day_sex = sample_day_before_fw_sex();
	m_miss_day[miss_cyc->beg_idx].prev = prev_day_sex - SEX_IMPUTE_SHIFT;

    }

    // each iteration samples `X_{ijk_r}` for the day corresponding to
    // `curr_miss_day`
    const int r_end = miss_cyc->beg_idx + miss_cyc->n_days;
    for (int r = miss_cyc->beg_idx; r < r_end; ++r) {

	const int curr_day_idx = m_miss_day[r].idx;

	// case: sex in the previous day was not missing, so the value of
	// `prev_day_sex` is given by the known value rather than whatever
	// value was previous sampled (i.e. the previous missing day was 2
	// or more days ago).
	if (! check_if_prev_sex_miss(r)) {
	    prev_day_sex = m_miss_day[r].prev;
	}
	// case: sex in the previous day was missing, so update it with the
	// latest sample, using the imputed sex coding shift
	else {
	    m_miss_day[r].prev = prev_day_sex - SEX_IMPUTE_SHIFT;
	}

	// case: W_{ijk_r} > 0, so intercourse must have occured on this day
	if ((curr_preg_idx != NON_PREG_CYC) && w_vals[curr_preg_idx++]) {
	    m_vals[curr_day_idx] = prev_day_sex = SEX_YES;
	}
	// case: W_{ijk_r} == 0, so we need to sample intercourse
	else {
	    // calculate `P(X_{ijk_r} = 1 | X_{ij{k_r-1}})`
	    prior_prob_yes = calc_prior_prob(utau, r, prev_day_sex);

	    // calculate `P(W_{ijk_r} = 0 | X_{ijk_r} = 1)`
	    posterior_prob_yes = calc_posterior_prob(ubeta, xi_i, curr_day_idx);

	    // sample `X_{ijk_r}`
	    m_vals[curr_day_idx] = prev_day_sex = sample_x_ijk(prior_prob_yes,
							       posterior_prob_yes);
	}
    }
}




inline double XGen::calc_prior_prob(const UProdTau& utau,
				    const int miss_day_idx,
				    const int prev_day_sex) const {

    const double* utau_vals = utau.vals();
    return prev_day_sex ?
	1 / (1 + exp(-utau_vals[miss_day_idx] - m_sex_coef)) :
	1 / (1 + exp(-utau_vals[miss_day_idx]));
}




inline double XGen::calc_posterior_prob(const UProdBeta& ubeta,
					const double xi_i,
					const int day_idx) {

    const double* ubeta_exp_vals = ubeta.exp_vals();
    return exp(-xi_i * ubeta_exp_vals[day_idx]);
}




inline int XGen::sample_x_ijk(const double prior_prob_yes,
			      const double posterior_prob_yes) {

    double prob_sex_no, prob_sex_yes, u;

    prob_sex_no = 1 - prior_prob_yes;
    prob_sex_yes = prior_prob_yes * posterior_prob_yes;

    u = R::unif_rand() * (prob_sex_no + prob_sex_yes);

    return (u < prob_sex_no) ? 0 : 1;
}




inline int XGen::sample_day_before_fw_sex() const {
    return (R::unif_rand() < m_cohort_sex_prob) ? 1 : 0;
}




// previous day intercourse is coded as 0 and 1 for days in which intercourse
// was observed.  When they are imputed, the value of 0 or 1 that is imputed is
// stored by shifting it by `SEX_IMPUTE_SHIFT` so that it can be distinguished
// from non-imputed values.  Thus if the value is negative it was imputed (and
// hence was missing), otherwise it was not imputed.

inline bool XGen::check_if_prev_sex_miss(int miss_day_idx) const {
    return (m_miss_day[miss_day_idx].prev < 0);
}




// calculate the prior probabilities `P(X_ijk | X_{ij{k-1}})` for each day of
// missing intercourse data in the cycle, and store in `prior_probs`.  The
// return value is the intercourse status for the day before the first missing
// day of intercourse in the cycle.
//
// In more detail, `prior_probs` is expected to have storage allocated for a
// table with two columns, and the number of rows given by the number of missing
// intercourse days in the cycle.  The 0-th column provides the prior
// probability for the corresponding day given that the intercourse status for
// the previous day was "no", while the 1-th column provides the prior
// probability for the corresponding day given that the intercourse status for
// the previous day was "yes."  Thus,
//
//     prior_probs[r][j] = P(X_{ijk_r} = 1 | X_{ij{k_r-1}} = j)
//
//         = 1 / (1 + exp(u_{ijk_r}^T tau)
//
// i.e. the logistic regression model where `tau` is a known set of regression
// coefficients.
//
// Now, some of the time the intercourse status for the day before is known
// (i.e. is nonmissing), and when this is the case then we will only fill in the
// appropriate column for the corresponding row.  On the other hand, when the
// previous day is missing, then we have to fill in both columns for the
// appropriate row.
//
// One complication is what to do with a missing value for intercourse for the
// day before the first day in the cycle.  What is done is to sample this value.
// Then the return value then is either the true value of the day before the
// first missing day of intercourse in the cycle if it is known, or the sampled
// value if not known.

// int XGen::calc_prior_probs(double prior_probs[][2],
// 			   const PregCyc* curr_miss_cyc,
// 			   const XMissDay* curr_miss_day,
// 			   const UProdTau& utau) const {

//     const double* utau_vals = utau.vals();
//     const int curr_beg_idx = curr_miss_cyc->beg_idx;
//     int day_before_fw_sex;

//     // perform the calculations for the first missing element.  The reason that
//     // we handle this seperately is because how we deal with a missing
//     // intercourse value from the day before the fertile window begins differs
//     // from how we deal with it for other days.  If this intercourse value is
//     // indeed missing then we sample it before sampling the rest of the data,
//     // based upon `P(X_0 = 1) = m_cohort_sex_prob`.  Whether or not the value
//     // needs to be sampled, the intercourse status for the day before the first
//     // missing intercourse day in the cycle is stored in the variable
//     // `day_before_fw_sex`.
//     //
//     // Note that the first missing day in the cycle need not be the first day in
//     // the cycle.  But if that's the case then it won't have a missing previous
//     // day of intercourse, so we don't need to consider a special case for that
//     // circumstance. Thus whatever the cycle day that the first missing
//     // observation may be, after this procedure, that day before it always have
//     // a nonmissing previous intercourse value stored in `day_before_fw_sex`.

//     if ((curr_miss_day->prev == SEX_NO) ||
// 	((curr_miss_day->prev == SEX_MISS) && (R::unif_rand() > m_cohort_sex_prob))) {
// 	day_before_fw_sex = 0;
//     }
//     else {
// 	day_before_fw_sex = 1;
//     }

//     // calculate the prior probability for the first day with missing
//     // intercourse, and store in the appropriate location in the prior
//     // probabilities table
//     switch (day_before_fw_sex) {
//     case (0):
// 	prior_probs[0][0] = 1 / (1 + exp(utau_vals[curr_beg_idx]));
// 	day_before_fw_sex = 0;
// 	break;
//     case (1):
// 	prior_probs[0][1] = 1 / (1 + exp(utau_vals[curr_beg_idx] + utau.sex_coef()));
// 	day_before_fw_sex = 1;
// 	break;
//     }

//     // calculate the remaining prior probabilities and store in the appropriate
//     // locations in the prior probabilities table.  Note that now it's possible
//     // for the value for intercourse in the preceeding day to also be missing,
//     // and when this is the case, we calculate the prior probabilities for both
//     // values.
//     for (int k = 0; k < curr_miss_cyc->n_days; ++k) {

// 	switch (curr_miss_day[k].prev) {
// 	case SEX_NO:
// 	    prior_probs[k][0] = 1 / (1 + exp(utau_vals[curr_beg_idx + k]));
// 	    break;
// 	case SEX_YES:
// 	    prior_probs[k][1] = 1 / (1 + exp(utau_vals[curr_beg_idx + k] + utau.sex_coef()));
// 	    break;
// 	case SEX_MISS:
// 	    prior_probs[k][0] = 1 / (1 + exp(utau_vals[curr_beg_idx + k]));
// 	    prior_probs[k][1] = 1 / (1 + exp(utau_vals[curr_beg_idx + k] + utau.sex_coef()));
// 	    break;
// 	}
//     }

//     return day_before_fw_sex;
// }




// int XGen::calc_posterior_probs(double* posterior_probs,
// 			       const PregCyc* curr_miss_cyc,
// 			       const XMissDay* curr_miss_day,
// 			       const double prior_probs[][2],
// 			       const WGen& W,
// 			       const XiGen& xi,
// 			       const UProdBeta& ubeta,
// 			       int day_before_fw_sex) {

//     // the bit shift operator calculates `2^{ curr_miss_cyc->n_miss_days }`.
//     // Thus we set aside enough memory to store the unnormalized posterior
//     // probability of every possible realization of the missing intercourse
//     // terms in the current cycle.
//     const int n_perms = 1 << curr_miss_cyc->n_days;

//     // calculate the `sum_{k: nonrand} x_ijk * exp(u_{ijk}^T beta)`, i.e. the
//     // nonrandom part of the sum that stays constant over the various
//     // permutations of the random parts of `X`.
//     const double* ubeta_exp_vals = ubeta.exp_vals();
//     // const double nonrand_sum_exp_ubeta = calc_nonrand_sum_exp_ubeta(curr_miss_cyc,
//     // 								    curr_miss_day,
//     // 								    ubeta);

//     // a bitmap where the k-th bit indicates whether `W_ijk_r > 0`
//     const int w_bitmap = get_w_bitmap(W, curr_miss_cyc, curr_miss_day);

//     const double xi_i = *(xi.vals() + curr_miss_cyc->subj_idx);

//     // Each iteration `t` indexes a permutation of a vector of 0's and 1's from
//     // among the possible realizations of missing itercourse data for the
//     // `ij`-th cycle.  Then for each of the `x_{ijk_r}` elements in the vector
//     // of missing intercourse data `(x_{ijk_1}, ..., x_{ijk_d})`, the
//     // calculations
//     //
//     //     P(W_{ijk_r} | x_{ijk_r}, exp(u_{ijk_r}^T beta))
//     //
//     // and
//     //
//     //     P(X_{ijk_r} = x_{ijk_r} | X_{ij{k_r-1}})
//     //
//     // are performed / obtained, from which we can calculate the unnormalized
//     // posterior probability of the `t`-th permutation.
//     //
//     // The permutation is determined by filling in the 0-th missing day with the
//     // value of the 0-th bit of the binary representation of `t`, the 1-th
//     // missing day with the value of the of the 1-th bit of the binary
//     // representation of `t`, and so on.

//     for (int t = 0; t < n_perms; ++t) {

// 	// // case: this permutation has an `X_ijk = 0` on a day when `W_ijk > 0`,
// 	// // which cannot happen
// 	// if (curr_miss_cyc->preg && ((w_bitmap & t) != w_bitmap)) {
// 	//     posterior_probs[t] = 0;
// 	//     continue;
// 	// }

// 	double sum_means;
// 	double prod_priors;
// 	// double pois_mean;
// 	int curr_x;

// 	// perform the calculations for the first missing element.  The reason
// 	// that we handle this seperately is because for the first element we
// 	// are guaranteed to have a nonmissing value of intercourse for the
// 	// previous day, and its status is stored in the variable
// 	// `day_before_fw_sex`.

// 	// P(W_{ijk_r} > 1 | X_{ijk_r}) is proportional to I(X_{ijk_r})
// 	//
// 	//                                 / 1,  X_{ijk_r} = 0
// 	// P(W_{ijk_r} = 0 | X_{ijk_r}) = |
// 	//                                 | a,  X_{ijk_r} = 1
// 	//
// 	// where a is the mean of the Poisson distribution, i.e.
// 	//
// 	//     a := exp{ -xi_i * exp(u_{ijk_r}^T beta) }

// 	// case: `x_{ijk_1}` = 1 for the `t`-th permutation
// 	if (t & 1) {
// 	    sum_means = (w_bitmap & 1) ? 0 : ubeta_exp_vals[curr_miss_day[0].idx];
// 	    prod_priors = prior_probs[0][day_before_fw_sex];
// 	    curr_x = 1;
// 	}
// 	// case: x_{ijk_1} = 0 for the `t`-th permutation.  Note that the prior
// 	// probability is `1 - P(X_{ijk_1} = 1 | X_{ij{k_1-1}})`.
// 	else {
// 	    prod_priors = 1 - prior_probs[0][day_before_fw_sex];
// 	    curr_x = 0;
// 	}

// 	// perform the calculations for the remaining missing elements.  Now it
// 	// is possible that the previous day of itercourse was missing.  If it
// 	// was not missing, then we find its value using `curr_miss_day[r].prev`
// 	// and look up the corresponding prior probability in the `prior_probs`
// 	// table.  If it was missing, then it must have been the immediately
// 	// preceeding value of intercourse under the `t`-th permutation, which
// 	// is stored as `curr_x`.

// 	for (int r = 1; r < curr_miss_cyc->n_days; ++r) {

// 	    // case: x_{ijk_r} = 1 for the current permutation
// 	    if (t & (1<<r)) {
// 		if (w_bitmap & (1<<r)) {
// 		    sum_means += ubeta_exp_vals[curr_miss_day[r].idx];
// 		}
// 		prod_priors *= (curr_miss_day[r].prev != SEX_MISS) ?
// 		    prior_probs[r][curr_miss_day[r].prev] :
// 		    prior_probs[r][curr_x];
// 		curr_x = 1;
// 	    }
// 	    // case: x_{ijk_r} = 0 for the current permutation.  Note that the
// 	    // probability is `1 - P(X_{ijk_r} = 1 | X_{ij{k_r-1}})`.
// 	    else {
// 		prod_priors *= (curr_miss_day[r].prev != SEX_MISS) ?
// 		    1 - prior_probs[r][curr_miss_day[r].prev] :
// 		    1 - prior_probs[r][curr_x];
// 		curr_x = 0;
// 	    }
// 	}

// 	// // Now calculate the unnormalized posterior probability for the `t`-th
// 	// // permutation.  The first step in the random variable case calculates
// 	// //
// 	// //     a := xi_i * sum_r x_{ijk_r} * exp(u_{ijk_r}^T beta)
// 	// //
// 	// // The second step calculates `P(Y_ij = y_ij | data)`, where
// 	// //
// 	// //     P(Y_ij = 1 | data) = 1 - exp(-a)
// 	// //
// 	// // Then in the third step, the unnormalized posterior probability is
// 	// // given by
// 	// //
// 	// //     P(Y_ij = y_ij | data) P(X_ij | data)
// 	// //
// 	// //         = P(Y_ij = y_ij | data) prod_k { P(X_ijk | X_{ij{k-1}}) }

// 	// // case: at least one day with intercourse that occurred during the
// 	// // cycle, and hence `Y_ij` is a random variable.
// 	// if (sum_exp_ubeta > 0) {
// 	//     sum_exp_ubeta *= *(xi.vals() + curr_miss_cyc->subj_idx);
// 	//     double prob_y_ij = (curr_miss_cyc->preg) ?
// 	// 	1 - std::exp(-sum_exp_ubeta) :
// 	// 	std::exp(-sum_exp_ubeta);
// 	//     posterior_probs[t] = prob_y_ij * prod_priors;
// 	// }
// 	// // case: no days of intercourse occurred during the cycle under the
// 	// // `t`-th permutation of missing intercourse values, so no pregnancy can
// 	// // possibly occur.
// 	// else {
// 	//     posterior_probs[t] = curr_miss_cyc->preg ? 0 : 1;
// 	// }

// 	posterior_probs[t] = exp(-xi_i * sum_means) * prod_priors;
//     }

//     return n_perms;
// }




// // Sampling from a multinomial distribution with n bins and 1 trial.  The idea
// // is to sample a uniform random variable and find which bin it falls into,
// // where the bins are as shown below.
// //
// //    probs[0]    probs[1]    probs[2]                    probs[n-1]
// //  /         | /         | /         |                 /           |
// // |-----------|-----------|-----------|----  ...  ----|-------------|

// inline int XGen::sample_x_perm(double* probs, int n_perms) {

//     double sum_probs, u;
//     int k;

//     // sum of the unnormalized probabilities
//     sum_probs = std::accumulate(probs, probs + n_perms, 0.0);

//     // sample from a `unif(0, sum_probs)` distribution
//     u = R::unif_rand() * sum_probs;

//     // find the bin that `u` falls into
//     sum_probs = *probs++;
//     k = 0;
//     while (u > sum_probs) {
// 	sum_probs += *probs;
// 	++probs;
// 	++k;
//     }

//     return k;
// }




// // fill in the updated values of X for the current cycle.  Fills in the 0-th
// // missing day with the value of the 0-th bit of the binary representation of
// // `t`, the 1-th missing day with the value of the of the 1-th bit of the binary
// // representation of `t`, and so on.

// inline void XGen::update_cyc_x(const PregCyc* curr_miss_cyc,
// 			       const XMissDay* curr_miss_day,
// 			       int t) {

//     for (int r = 0; r < curr_miss_cyc->n_days; ++r) {
// 	m_vals[curr_miss_day[r].idx] = t & (1<<r) ? 1 : 0;
//     }
// }




// // inline double XGen::calc_nonrand_sum_exp_ubeta(const PregCyc* curr_miss_cyc,
// // 					       const XMissDay* curr_miss_day,
// // 					       const UProdBeta& ubeta) {

// //     const double* ubeta_exp_vals = ubeta.exp_vals();
// //     double sum_val = 0;
// //     int curr_idx = curr_miss_cyc->beg_idx;
// //     int end_idx = curr_idx + curr_miss_cyc->n_days;

// //     for ( ; curr_idx < end_idx; ++curr_idx) {

// // 	// case: the current index isn't a missing value for `X_ijk`.  Note that
// // 	// guarantees that it has a value of 1 or else it wouldn't be in the
// // 	// data.
// // 	if (curr_idx != curr_miss_day->idx) {
// // 	    sum_val += ubeta_exp_vals[curr_idx];
// // 	}
// // 	// case: the current index is a missing value for `X_ijk`.  Don't do
// // 	// anything with `sum_val`, and increment `curr_miss_day` so as to look
// // 	// for the next day with missing intercourse data.
// // 	else {
// // 	    ++curr_miss_day;
// // 	}
// //     }

// //     return sum_val;
// // }




// inline int XGen::get_w_bitmap(const WGen& W,
// 			      const PregCyc* curr_miss_cyc,
// 			      const XMissDay* curr_miss_day) {

//     const int* w_vals = W.vals();
//     int w_bitmap = 0;

//     for (int r = 0; r < curr_miss_cyc->n_days; ++r) {
// 	if (w_vals[curr_miss_day[r].idx]) {
// 	    w_bitmap |= (1<<r);
// 	}
//     }

//     return w_bitmap;
// }
