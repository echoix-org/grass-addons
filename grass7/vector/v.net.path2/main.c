
/****************************************************************
 *
 * MODULE:       v.net.path2
 * 
 * AUTHOR(S):    Radim Blazek
 *               
 * PURPOSE:      Shortest path on vector network
 *               
 * COPYRIGHT:    (C) 2002, 2013 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 ****************************************************************/
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

int path(struct Map_info *, struct Map_info *, char *, int, double, int, int,
	 int, int);

int main(int argc, char **argv)
{
    struct Option *input_opt, *output_opt, *afield_opt, *nfield_opt,
	*tfield_opt, *tucfield_opt, *afcol, *abcol, *ncol, *type_opt;
    struct Option *max_dist, *file_opt;
    struct Flag *geo_f, *segments_f, *turntable_f;
    struct GModule *module;
    struct Map_info In, Out;
    int type, afield, nfield, tfield, tucfield, geo;
    double maxdist;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("network"));
    G_add_keyword(_("shortest path"));
    module->description = _("Finds shortest path on vector network.");

    input_opt = G_define_standard_option(G_OPT_V_INPUT);
    output_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "line,boundary";
    type_opt->answer = "line,boundary";
    type_opt->label = _("Arc type");

    afield_opt = G_define_standard_option(G_OPT_V_FIELD);
    afield_opt->key = "alayer";
    afield_opt->answer = "1";
    afield_opt->label = _("Arc layer");

    nfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    nfield_opt->key = "nlayer";
    nfield_opt->answer = "2";
    nfield_opt->label = _("Node layer");

    tfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    tfield_opt->key = "tlayer";
    tfield_opt->answer = "3";
    tfield_opt->label = _("Turntable layer");

    tucfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    tucfield_opt->key = "tuclayer";
    tucfield_opt->answer = "4";
    tucfield_opt->label = _("Unique categories layer for turntable");

    file_opt = G_define_standard_option(G_OPT_F_INPUT);
    file_opt->key = "file";
    file_opt->required = NO;
    file_opt->description = _("Name of file containing start and end points. "
			      "If not given, read from stdin");


    afcol = G_define_option();
    afcol->key = "afcolumn";
    afcol->type = TYPE_STRING;
    afcol->required = NO;
    afcol->description = _("Arc forward/both direction(s) cost column");

    abcol = G_define_option();
    abcol->key = "abcolumn";
    abcol->type = TYPE_STRING;
    abcol->required = NO;
    abcol->description = _("Arc backward direction cost column");

    ncol = G_define_option();
    ncol->key = "ncolumn";
    ncol->type = TYPE_STRING;
    ncol->required = NO;
    ncol->description = _("Node cost column");

    max_dist = G_define_option();
    max_dist->key = "dmax";
    max_dist->type = TYPE_DOUBLE;
    max_dist->required = NO;
    max_dist->answer = "1000";
    max_dist->label = _("Maximum distance to the network");
    max_dist->description = _("If start/end are given as coordinates. "
			      "If start/end point is outside this threshold, "
			      "the path is not found "
			      "and error message is printed. To speed up the process, keep this "
			      "value as low as possible.");

    geo_f = G_define_flag();
    geo_f->key = 'g';
    geo_f->description =
	_("Use geodesic calculation for longitude-latitude locations");

    segments_f = G_define_flag();
    segments_f->key = 's';
    segments_f->description = _("Write output as original input segments, "
				"not each path as one line.");

    turntable_f = G_define_flag();
    turntable_f->key = 't';
    turntable_f->description = _("Use turntable"
				 "(otherwise tuclayer and tlayer are ignored)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    type = Vect_option_to_types(type_opt);
    afield = atoi(afield_opt->answer);
    nfield = atoi(nfield_opt->answer);
    tfield = atoi(tfield_opt->answer);
    tucfield = atoi(tucfield_opt->answer);
    maxdist = atof(max_dist->answer);

    if (geo_f->answer) {
	geo = 1;
	if (G_projection() != PROJECTION_LL)
	    G_warning(_("The current projection is not longitude-latitude"));
    }
    else
	geo = 0;

    Vect_check_input_output_name(input_opt->answer, output_opt->answer,
				 G_FATAL_EXIT);

    Vect_set_open_level(2);
    Vect_open_old(&In, input_opt->answer, "");

    if (1 > Vect_open_new(&Out, output_opt->answer, Vect_is_3d(&In))) {
	Vect_close(&In);
	G_fatal_error(_("Unable to create vector map <%s>"),
		      output_opt->answer);
    }
    Vect_hist_command(&Out);

    if (turntable_f->answer)
	Vect_net_ttb_build_graph(&In, type, afield, nfield, tfield, tucfield,
				 afcol->answer, abcol->answer, ncol->answer,
				 geo, 0);
    else
	Vect_net_build_graph(&In, type, afield, nfield, afcol->answer,
			     abcol->answer, ncol->answer, geo, 0);

    path(&In, &Out, file_opt->answer, nfield, maxdist, segments_f->answer,
	 tfield, tucfield, turntable_f->answer);

    Vect_close(&In);

    Vect_build(&Out);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
