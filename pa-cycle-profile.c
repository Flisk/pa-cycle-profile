#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pulse/pulseaudio.h>

#include "strlist.h"

struct program_context {
        /* flags & arguments */
        int debug;
        const char *card_name;
        struct strlist *profile_names;
        size_t n_profile_names;

        /* internals */
        pa_mainloop *mainloop;
};

/* program lifecycle */
static void on_state_change(pa_context *, void *);
static void on_card_info(pa_context *, const pa_card_info *, int, void *);
static void on_profile_change_result(pa_context *, int, void *);

/* auxilary */
static void parse_args(struct program_context *, int, char **);
static void usage(FILE *);
static void panic(int, pa_context *, const char *, ...);

char *program_name = NULL;

static void parse_args(struct program_context *pc, int argc, char **argv)
{
        for (ssize_t i = 1; i < argc; i++) {
                const char *arg = argv[i];

                if (arg[0] == '-') {
                        if (strcmp(arg, "-d") == 0 || strcmp(arg, "--debug") == 0) {
                                pc->debug = 1;
                                continue;
                        } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
                                usage(stdout);
                                exit(EXIT_SUCCESS);
                        }

                        panic(1, NULL, "unknown flag: %s", arg);
                }

                if (pc->card_name == NULL) {
                        pc->card_name = arg;
                        continue;
                }

                if (strlist_append(pc->profile_names, arg) < 0) {
                        panic(0, NULL, "strlist_append ran out of memory");
                }

                pc->n_profile_names += 1;
        }

        if (pc->card_name == NULL || pc->n_profile_names < 2) {
                panic(1, NULL, "too few arguments");
        }
}

static void usage(FILE *out)
{
        fprintf(
                out,
                "usage: %s [-d|--debug] [-h|--help] <card name> <profile> <profile> [<profile>...]\n"
                "\t-d, --debug    enable debug output\n"
                "\t-h, --help     print this help message\n",
                program_name
        );
}

static void panic(int print_usage, pa_context *context, const char *format, ...)
{
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);

        if (context != NULL) {
                int pulse_errno = pa_context_errno(context);
                const char *pulse_msg = pa_strerror(pulse_errno);

                fprintf(stderr, ": %s", pulse_msg);
        }

        fputc('\n', stderr);

        if (print_usage) {
                usage(stderr);
        }

        exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
        program_name = basename(argv[0]);

        struct strlist profile_names = { 0 };
        struct program_context pc = { 0 };

        pc.profile_names = &profile_names;

        parse_args(&pc, argc, argv);

	pa_mainloop *mainloop = pa_mainloop_new();
	pa_mainloop_api *mainloop_api = pa_mainloop_get_api(mainloop);
	pa_context *context = pa_context_new(mainloop_api, program_name);

        pc.mainloop = mainloop;

	pa_context_set_state_callback(context, on_state_change, &pc);

        if (pa_context_connect(context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) {
                panic(0, context, "failed to connect");
        }

        if (pa_mainloop_run(mainloop, NULL) < 0) {
                panic(0, context, "failed to run mainloop");
        }

	exit(EXIT_SUCCESS);
}

static void on_state_change(
	pa_context			*context,
	void				*userdata
) {
        pa_context_state_t state = pa_context_get_state(context);
        struct program_context *pc = (struct program_context *) userdata;

        switch (state) {
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
                if (pc->debug) {
                        fprintf(stderr, "state = %d\n", state);
                }
                break;

        case PA_CONTEXT_READY:
                pa_context_get_card_info_by_name(context, pc->card_name, on_card_info, userdata);
                break;

        case PA_CONTEXT_FAILED:
                panic(0, context, "unknown error");
                break;

        case PA_CONTEXT_TERMINATED:
                if (pc->debug) {
                        fprintf(stderr, "quitting main loop\n");
                }
                pa_mainloop_quit(pc->mainloop, 0);
                break;

        default:
                panic(0, NULL, "unhandled state change (state = %d)\n", state);
        }
}

static void on_card_info(
	pa_context		*context,
	const pa_card_info	*info,
	int			 eol,
	void			*userdata
) {
        struct program_context *pc = (struct program_context *) userdata;

        if (eol == -1) {
                panic(0, context, "error while fetching card %s", pc->card_name);
        }

        if (eol == 1) {
                return;
        }

        struct strlist_node *node = pc->profile_names->head;

        while (node) {
                if (strcmp(node->s, info->active_profile->name) == 0) {
                        goto found;
                }

                node = node->next;
        }

        panic(0, NULL, "active profile (%s) is not in the argument list", info->active_profile->name);

found:
        /* the C standard requires labels to be paired with a
           statement, which declarations are not, so... */
        ;

        struct strlist_node *next_profile_node =
                node->next == NULL ? pc->profile_names->head : node->next;

        if (pc->debug) {
                fprintf(
                        stderr,
                        "active profile is %s, switching to %s\n",
                        info->active_profile->name,
                        next_profile_node->s
                );
        }

        pa_context_set_card_profile_by_name(
                context,
                pc->card_name,
                next_profile_node->s,
                on_profile_change_result,
                userdata
        );
}

static void on_profile_change_result(pa_context *context, int success, void *userdata) {
        struct program_context *pc = (struct program_context *) userdata;
        if (pc->debug) {
                fprintf(stderr, "success = %d\n", success);
        }

        if (success != 1) {
                panic(0, context, "couldn't switch profile");
        }

        pa_context_disconnect(context);
}

