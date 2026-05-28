#pragma once

#if defined(INDOOR_UWB_ROLE_TAG)
const char indoor_html[] PROGMEM =
	"<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Indoor "
	"MAP</title></head><body></body><script "
	"src=\"https://indooruwb.s3.eu-west-3.amazonaws.com/indoor_map_manager.js\">"
	"</script></html>";
const char node_html[] PROGMEM =
	"<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Anchor Indoor "
	"Config</title><link "
	"href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.2.0-beta1/dist/css/"
	"bootstrap.min.css\" rel=\"stylesheet\"><script "
	"src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js\">"
	"</script><script "
	"src=\"https://cdnjs.cloudflare.com/ajax/libs/json2html/2.1.0/json2html.min."
	"js\"></script><script "
	"src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.2.0-beta1/dist/js/bootstrap."
	"bundle.min.js\"></script></head><body></body><script "
	"src=\"https://indooruwb.s3.eu-west-3.amazonaws.com/anchorlist_loader.js\">"
	"</script></html>";
#endif

#if defined(INDOOR_UWB_ROLE_ANCHOR)
const char anchor_html[] PROGMEM =
	"<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Indoor "
	"Anchor</title></head><body></body><script "
	"src=\"https://indooruwb.s3.eu-west-3.amazonaws.com/indoor_anchor_manager.js\">"
	"</script></html>";
#endif
