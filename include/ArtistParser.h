#ifndef ARTISTPARSER_H
#define ARTISTPARSER_H

#include <string>
#include <vector>

/*
 * ArtistParser — Multi-artist extraction from YouTube titles
 *
 * YouTube video titles often encode multiple artists via
 * separators ("ft.", "feat.", "&", "x") or via a " — "
 * split where one side is the artist and the other the
 * song title.  These helpers attempt to reconstruct the
 * real artist list heuristically.
 */

/* Split a string on common feature / collaboration separators */
std::vector<std::string> parse_artists(const std::string& author);

/* Strip suffixes like " — Topic", " and N more" */
std::string clean_artist_name(std::string name);

/* Reject strings that look like video keywords or contain brackets */
bool is_valid_artist_name(const std::string& name);

/*
 * Split a video title on " — " and use the uploader name
 * + keyword heuristics to decide which side is the artist.
 * Also extracts "ft." / "feat." features from the song side.
 */
std::vector<std::string> extract_artists_from_title(
    const std::string& title,
    const std::string& cleaned_uploader);

/*
 * Show a small dialog listing multiple artist candidates
 * and let the user pick one.  Returns the chosen index
 * or -1 if cancelled.
 */
int show_artist_selector(const std::vector<std::string>& artists);

#endif // ARTISTPARSER_H
