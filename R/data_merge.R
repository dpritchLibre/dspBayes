# merge the data provided by `baseline`, `cycle`, and `daily` and return as a
# data frame.  In more detail, `baseline` is merged over the subject ID
# variable, and `subject` is merged over both the subject ID and also the cycle
# number.  Unused variables are not included in the merged data, nor are days
# outside of the fertile window.  Days inside the fertile window that are not
# included in the original data are added to the merged data with missing values
# inserted for the remaining variables when they are not known.  If the number
# of days in the fertile window for a cycle is smaller than `req_min_days`, then
# the cycle is removed from the data. The returned data is sorted over subject
# ID, cycle number, and fertile window day, in that order.
#
# PRE: `baseline`, `cycle`, and `daily` are data frames or objects that can be
# coerced to a data frame.  `var_nm` is an object created by `extract_var_nm`.
# `fw_incl` is a vector giving the fertile window days, and `req_min_days` is
# the minimum number of days needed in the data to include a cycle.

merge_dsp_data <- function(baseline,
                           cycle,
                           daily,
                           var_nm,
                           fw_incl,
                           fw_day_before,
                           use_na,
                           req_min_days) {

    # # conditionally create some variables that track where baseline- and
    # # cycle-specific variables end up after merging
    # if ((use_na == "covariates") || (use_na == "all")) {

    #     # add to `var_nm` to prevent from being dropped
    #     var_nm$all <- c(var_nm$all, "track_base__", "track_cyc__")

    #     # tracking variables
    #     if (! is.null(baseline)) {
    #         baseline$track_base__ <- NROW(baseline) %>% seq_len
    #     }
    #     if (! is.null(cycle)) {
    #         cycle$track_cyc__ <- NROW(cycle) %>% seq_len
    #     }
    # }

    # reduce each dataset to only contain the variables that we need
    # (i.e. subsets the columns, and rows remain unchanged).  Returns NULL if
    # the input is NULL.
    base_red <- get_red_dat(baseline, var_nm)
    cyc_red <- get_red_dat(cycle, var_nm)
    day_red <- get_red_dat(daily, var_nm)

    # obtain every unique subject id and cycle pair for any cycle that has
    # pregnancy outcome data
    keypairs <- get_keypairs(day_red, cyc_red, var_nm)

    # conditionally add a column in the daily data providing "intercourse the
    # day before" information
    if (! is.null(fw_day_before)) {
        day_red <- daily_add_sex_yester(day_red, var_nm, fw_incl, fw_day_before)
    }

    # # conditionally add columns in the daily data used later to track missing
    # # intercourse
    # if ((use_na == "sex") || (use_na == "all")) {
    #     day_red <- create_xmiss_var(day_red, var_nm)
    # }

    # reduce daily dataset to only include rows corresponding to days in the
    # fertile window.  Note that we must do this after obtaining `keypairs`,
    # because otherwise we could lose cycles that have pregnancy information but
    # where none of the fertile window days are recorded.
    keep_idx <- day_red[[var_nm$fw]] %in% fw_incl %>% which
    day_red <- day_red[keep_idx, ]
    # TODO: verify that there are no duplicate (id, cyc, day) tuples
    # TODO: check if we threw out pregnancy cycles without any intercourse and throw a warning?

    # a data frame with observations given by the cross product of every day in
    # the fertile window and every (id, cycle) keypair
    comb_dat <- construct_day_obs(day_red, keypairs, fw_incl, var_nm, req_min_days)

    # join operation on id and cycle.  `all.x = TRUE` specifies that we keep
    # every observation in `day_formatted`.  Under these specifications `merge`
    # throws away observations in `cyc_red` that don't have matches, which is
    # what we want since `day_formatted` already has all of the id and cycle
    # pairs with pregnancy information.  We defer sorting the joined data until
    # later.
    if (! is.null(cyc_red)) {
        comb_dat <- merge(x     = comb_dat,
                          y     = cyc_red,
                          by    = c(var_nm$id, var_nm$cyc),
                          all.x = TRUE,
                          sort  = FALSE)
    }

    # conditionally merge baseline in with cycle and daily.  See above merge for
    # discussion of the parameter settings.
    if (! is.null(base_red)) {
        comb_dat <- merge(x     = comb_dat,
                          y     = base_red,
                          by    = var_nm$id,
                          all.x = TRUE,
                          sort  = FALSE)
    }

    # drop unused factor levels after subsetting
    for (i in seq_along(comb_dat)) {
        if (is.factor(comb_dat[[i]])) {
            comb_dat[[i]] <- droplevels(comb_dat[[i]])
        }
    }

    # return data sorted over id, cycle, and cycle day
    sort_dsp(comb_dat, var_nm, fw_incl)
}




# takes a data frame `dataset` and returns the data after removing any columns
# with names do not appear in `var_nm$all`
#
# PRE: `dataset` is a data.frame and `var_nm` is a list with an element `all`
# which is a character vector

get_red_dat <- function(dataset, var_nm) {

    # if data is NULL then function is a noop
    if (is.null(dataset)) {
        return(NULL)
    }

    # subset the data variables according to whether they are among the set of
    # all variables listed in the model or used to combine the data
    var_incl_bool <- colnames(dataset) %in% var_nm$all

    # we can't allow missing data for these fundamental columns, so create an
    # index of rows to remove
    critical_cols_bool <- colnames(dataset) %in% c(var_nm$id, var_nm$cyc, var_nm$fw, var_nm$preg)
    obs_incl_bool <- complete.cases(dataset[, critical_cols_bool, drop = FALSE])

    dataset[obs_incl_bool, var_incl_bool, drop = FALSE] %>%
        as.data.frame(., stringsAsFactors = FALSE)
}




# return a data frame with two columns, with the rows containing every unique
# subject id and cycle pair for any cycle that has pregnancy outcome data.
# `cycle` is allowed to be NULL.
#
# PRE: assumes `daily` and `cycle` are data frames (or cycle may be NULL), each
# having columns for id and cycle.  Either one or both of `daily` or `cycle`
# must also have a column for pregnancy status.  The names of each of these
# columns is given by `var_nm`.

get_keypairs <- function(daily, cycle, var_nm) {

    # create `keypairs_df` with all of the subject id and cycle pair for any
    # cycle that has pregnancy outcome data

    # case: both datasets contain pregnancy information
    if ((var_nm$preg %in% colnames(daily)) && (var_nm$preg %in% colnames(cycle))) {
        keypairs_df <- rbind(cycle[, c(var_nm$id, var_nm$cyc)],
                             daily[, c(var_nm$id, var_nm$cyc)],
                             stringsAsFactors = FALSE)
    }
    # case: only the daily data constains pregnancy information
    else if (var_nm$preg %in% colnames(daily)) {
        keypairs_df <- daily[, c(var_nm$id, var_nm$cyc)]
    }
    # case: only the cycle data constains pregnancy information
    else if (var_nm$preg %in% colnames(cycle)) {
        keypairs_df <- cycle[, c(var_nm$id, var_nm$cyc)]
    }
    # case: can't find the pregnancy status variable
    else {
        paste0("cannot find ", var_nm$preg, " in cycle or daily") %>% stop(call. = FALSE)
    }

    # obtain unique keypairs
    dup_keys_bool <- duplicated(keypairs_df)
    keypairs_df <- keypairs_df[! dup_keys_bool, ]

    # return keypairs sorted by id and cycle
    keypairs_df[order(keypairs_df[, var_nm$id], keypairs_df[, var_nm$cyc]), ]
}




# returns the data frame `daily`, with a column appended to it named
# `sex_yester`, which has the intercourse status of the day before the
# observation for days in the fertile window.  For days outside of the fertile
# window, the variable has a missing value.
#
# PRE: `daily` is a data frame with columns for id, cycle, cycle day, and
# intercourse status and with names as specified by `var_nm`.  It is assumed
# that there is no missing data in these variables, with the exception of
# intercourse.  `fw_include` is a vector with element specifying which values
# are in the fertile window, and `fw_day_before` is a single value specifying
# the day before the fertile window.

daily_add_sex_yester <- function(daily, var_nm, fw_incl, fw_day_before) {

    # sort data so that we need only inspect the `i - 1`-th observation to look
    # for the previous day's intercourse status
    daily <- sort_dsp(daily, var_nm, fw_incl, fw_day_before)

    # bind variable names for convenience
    id <- daily[[var_nm$id]]
    cyc <- daily[[var_nm$cyc]]
    cycleday <- daily[[var_nm$fw]]
    sex <- daily[[var_nm$sex]]

    # copy sex and "NA out" the data
    sex_yester <- sex
    sex_yester[seq_along(sex_yester)] <- NA

    # the fertile window with the day before prepended
    fw_extend <- c(fw_day_before, fw_incl)

    # each iteration looks to see if the i-th observation is in the fertile
    # window.  If so, then it looks to see if the `i - 1`-th observations is the
    # previous day in the fertile window, and if so then records the intercourse
    # status for that day.
    for (i in 2L:length(cycleday)) {

        # note: we assume that there is never more than 1 match here
        fw_idx <- match(cycleday[i], fw_incl)

        # case: a match was found
        if (! is.na(fw_idx)) {

            # case: the day before in the data is the previous day in the
            # fertile window of the same subject and cycle.  Note: we assume
            # that there is no missing data in these variables.
            if ((cycleday[i - 1] == fw_extend[fw_idx]) &&
                (cyc[i - 1L] == cyc[i]) &&
                (id[i - 1L] == id[i])) {

                sex_yester[i] <- sex[i - 1L]
            }
        }
    }

    daily$sex_yester <- sex_yester
    daily
}




# construct the day-specific part of the data.  Returns a data frame with
# observations given by the cross product of every day in the fertile window and
# every (id, cycle) keypair.
#
# PRE: assumes `daily` is a data frame with id, cycle and fertile window columns
# with names as given in `var_nm`.  `keypairs` is a nonempty data frame with
# exactly two columns for id and cycle.  `fw_incl` is a nonempty atomic
# vector. `var_nm` is a list providing the names of the id, cycle, and fertile
# window columns.  `req_min_days` is a length-1 numeric vector.

construct_day_obs <- function(daily, keypairs, fw_incl, var_nm, req_min_days) {

    # the length of fertile window and the number of cycles in the data
    fw_len <- length(fw_incl)
    n_cycle <- NROW(keypairs)

    # a data frame with vectors of the same types as in `daily` and number of
    # rows the same as the number of fertile window days.  All values are NA.
    template_df <- construct_template_df(daily, fw_incl)

    # container with each element a data frame representing the days in the
    # fertile window
    days_by_cyc <- vector("list", n_cycle)
    n_obs_by_cyc <- vector("logical", n_cycle)

    # each iteration constructs a data frame for the daily data corresponding to
    # current subject and cycle, and saves results to `days_by_cyc`
    for (k in 1:n_cycle) {

        # subject id and cycle for current keypair
        curr_id <- keypairs[k, var_nm$id]
        curr_cyc <- keypairs[k, var_nm$cyc]

        # tracks how many observations are nonmissing for the current subject and
        # cycle
        curr_n_obs <- 0L

        # copy empty data frame and fill in id, cycle, and fertile window day
        # data.  Note that `curr_id` and `curr_cyc` are scalars and rely on R to
        # recycle their values to fill the vector, while `fw_incl` is of the
        # appropriate length.
        curr_df <- template_df
        curr_df[[var_nm$id]] <- curr_id
        curr_df[[var_nm$cyc]] <- curr_cyc
        curr_df[[var_nm$fw]] <- fw_incl

        # TODO: this process can introduce missing into the pregnancy variable
        # if pregnancy is part of the daily data.  Have to think about how to
        # handle this.

        # reduce to daily to the current subject and cycle
        curr_day_idx <- (daily[[var_nm$id]] == curr_id &
                         daily[[var_nm$cyc]] == curr_cyc) %>% which
        curr_daily <- daily[curr_day_idx, ]

        # each iteration copies one row of data from `curr_daily` into the
        # correct row of `curr_df` if one exists, otherwise nothing is done
        for (i in seq_along(fw_incl)) {

            # row in `curr_daily` corresponding the fertile window day
            row_idx <- which(curr_daily[[var_nm$fw]] == fw_incl[i])

            # case: found exactly 1 match, copy contents from daily data into
            # current data frame
            if (length(row_idx) == 1L) {
                curr_df[i, ] <- curr_daily[row_idx, ]
                curr_n_obs <- curr_n_obs + 1L
            }
            # case: multiple matches, throw an error
            else if (length(row_idx) > 1L) {
                stop("multiple matches in daily data for subject ", curr_id,
                     " cycle ", curr_cyc, " fw day ", fw_incl[i],
                     ": dropping observation", call. = FALSE)
            }
            # else: 0 matches found.  Do nothing (i.e. leave an observation with
            # missing values in it)

        } # end copy day-specific data loop

        # save contents of daily data corresponding to current subject and cycle
        # to storage list
        days_by_cyc[[k]] <- curr_df
        n_obs_by_cyc[k] <- curr_n_obs

    } # end construct daily data for a given keypair loop

    # remove observations that don't meet the minimum number of observations
    # specified to keep
    days_by_cyc <- days_by_cyc[n_obs_by_cyc >= req_min_days]

    # rbind each of the elements in `days_by_cyc`, each of which are data frames
    # corresponding to the days for a given subject and cycle pair
    rbind_similar_dfs(days_by_cyc)
}




# construct a data frame with vectors of the same types and attributes as
# `daily`, number of rows the same length as `fw_incl`, and all values are NA
#
# PRE: `daily` is a data frame of at least one row and one column, and `fw_incl`
# is a nonempty atomic vector with length no bigger than the number of rows in
# `daily`

construct_template_df <- function(daily, fw_incl) {

    # obtain a data frame of the right size and vector types
    template_df <- daily[seq_along(fw_incl), , drop = FALSE]

    # fill all of the values with missing
    for (k in seq_along(template_df)) {
        # index all of the vector so that R keeps the vector internal structure
        # and attributes
        template_df[[k]][seq_along(fw_incl)] <- NA
    }

    row.names(template_df) <- seq_along(fw_incl)
    template_df
}




# combine a list of data frames that have a specific form.  It is assumed that
# each element of `list_of_dfs` is a data frame of exactly the same dimensions
# and attributes.  The function then performs an operation that is logically
# equivalent to `rbind()`ing each of these data frames
#
# PRE: `list_of_dfs` is a list with length > 0 such that each element is a data
# frame of exactly the same dimensions and attributes

rbind_similar_dfs <- function(list_of_dfs) {

    if (length(list_of_dfs) == 0L) {
        stop("no cycles passed the criteria", call. = FALSE)
    }

    # set data parameters
    n_dfs <- length(list_of_dfs)
    n_vars <- list_of_dfs[[1L]] %>% NCOL
    n_elem <- list_of_dfs[[1L]] %>% NROW
    vec_len <- n_dfs * n_elem

    # container to put each of the concatenated columns into
    combined_list <- vector("list", n_vars)
    names(combined_list) <- list_of_dfs[[1L]] %>% names

    # each iteration creates one variable that is the concatenation of the j-th
    # variable across all of the data frames
    for (j in seq_len(n_vars)) {

        # initialize current vector in the list
        combined_list[[j]] <- rep(list_of_dfs[[1L]][, j], n_dfs)

        # construct index { 2, 3..., n_dfs }.  Note that this results in an
        # integer(0) vector if `n_dfs` equals 1.
        df_idx <- seq_len(n_dfs)
        df_idx <- df_idx[-n_dfs] + 1L

        # each iteration fills in the values of the j-th variable of the k-th
        # data frame into the appropriate elements of the j-th variable in
        # `combined_list`
        for (k in df_idx) {

            # indices of elements to fill in for current vector
            curr_idx <- ((k - 1) * n_elem + 1) : (k * n_elem)

            # copy the j-th column of the k-th data frame into the appropriate
            # elements of the combined_list column
            combined_list[[j]][curr_idx] <- list_of_dfs[[k]][, j]
        }
    }

    data.frame(combined_list, stringsAsFactors = FALSE)
}




# sort over id / cycle / fertile window
#
# PRE: `comb_dat` is a data frame with id, cycle, and fertile window variables
# with names given by `var_nm`.  `fw_incl` provides the fertile window days.

sort_dsp <- function(comb_dat, var_nm, fw_incl, fw_day_before = NULL) {

    fw_extend <- c(fw_day_before, fw_incl)

    # turn fertile window vector into a factor if it's not already so that
    # `order` will give us the desired sorting
    out <- comb_dat[order(comb_dat[, var_nm$id],
                          comb_dat[, var_nm$cyc],
                          comb_dat[, var_nm$fw] %>% factor(., levels = fw_extend)), ]
    row.names(out) = NROW(comb_dat) %>% seq_len
    out
}




# create_xmiss_var <- function(daily, var_nm) {

#     # create a unique key for each observation that we can point to for missing
#     # values of the soon-to-be created `xmiss_` variable.  Later on, `xmiss_`
#     # will have a different structure from `daily`, so we need a way to map
#     # elements of `xmiss_` back to the main dataset.
#     daily$obs_idx_ <- seq_len(nrow(daily))

#     # creates a vector with -1L for "no", 0L for "yes", and NA for missing
#     daily$xmiss_ <- map_vec_to_bool(daily[[var_nm$sex]]) %>% as.integer %>% `-`(1L)

#     # replace the NA's in `miss_idx_` with the index of the missing, which will
#     # later be used to map back to the observations in the data through
#     # `daily$obs_idx_`
#     miss_idx <- which(is.na(daily$xmiss_))
#     daily$xmiss_[miss_idx] <- miss_idx

#     daily
# }
