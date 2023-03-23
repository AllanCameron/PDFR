utils::globalVariables(
  c(
    "X", "Y", "bottom", "box", "fill", "left", "midx", "midy", "poly", "right",
    "size", "stroke", "text", "top","xmax", "xmin", "ymax", "ymin"
  )
)

#' Error if pdf is not a valid input
#'
#' @param pdf Object to check
#' @keywords internal
#' @noRd
check_pdf <- function(pdf, call = caller_env()) {
  if (any(
    c(
      !is_raw(pdf) && is_false(is_character(pdf)),
      is_character(pdf) && is_min_length(pdf),
      is_character(pdf) && !is_pdf_fileext(pdf[1])
    )
  )) {
    cli_abort(
      "{.arg pdf} must be a single path to a valid pdf file or a raw vector
      string, not {.obj_type_friendly {pdf}}.",
      call = call
    )
  }
}

#' Does x end with a PDF file extension?
#'
#' @param x Object to check for PDF file extension.
#' @inheritParams base::grepl
#' @keywords internal
#' @noRd
is_pdf_fileext <- function(x, ignore.case = TRUE) {
  grepl("[.]pdf$", x, ignore.case = ignore.case)
}

#' Does x contain a file separator character?
#'
#' @param x Object to check for a file separator character
#' @param fsep File separator character. Defaults to `.Platform$file.sep`
#' @keywords internal
#' @noRd
is_fsep_path <- function(x, fsep = .Platform$file.sep) {
  grepl(fsep, x)
}

#' Does x end with a PDF file extension?
#'
#' @param x Object to check for minimum length.
#' @param n Minimum length to return `TRUE`.
#' @keywords internal
#' @noRd
is_min_length <- function(x, n = 2) {
  length(x) >= n
}
