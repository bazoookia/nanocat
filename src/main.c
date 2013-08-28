#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/bus.h>
#include <nanomsg/pair.h>
#include <nanomsg/survey.h>
#include <nanomsg/reqrep.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "options.h"

enum echo_format {
    NC_NO_ECHO,
    NC_ECHO_RAW,
    NC_ECHO_ASCII,
    NC_ECHO_QUOTED,
    NC_ECHO_MSGPACK
};

typedef struct nc_options {
    // Global options
    int verbose;

    // Socket options
    int socket_type;
    struct nc_string_list bind_addresses;
    struct nc_string_list connect_addresses;
    float send_timeout;
    float recv_timeout;
    struct nc_string_list subscriptions;

    // Data options
    struct nc_blob data_to_send;

    // Echo options
    enum echo_format echo_format;
} nc_options_t;

//  Constants to get address of in option declaration
static const int nn_push = NN_PUSH;
static const int nn_pull = NN_PULL;
static const int nn_pub = NN_PUB;
static const int nn_sub = NN_SUB;
static const int nn_req = NN_REQ;
static const int nn_rep = NN_REP;
static const int nn_bus = NN_BUS;
static const int nn_pair = NN_PAIR;
static const int nn_surveyor = NN_SURVEYOR;
static const int nn_respondent = NN_RESPONDENT;


struct nc_enum_item socket_types[] = {
    {"PUSH", NN_PUSH},
    {"PULL", NN_PULL},
    {"PUB", NN_PUB},
    {"SUB", NN_SUB},
    {"REQ", NN_REQ},
    {"REP", NN_REP},
    {"BUS", NN_BUS},
    {"PAIR", NN_PAIR},
    {"SURVEYOR", NN_SURVEYOR},
    {"RESPONDENT", NN_RESPONDENT},
    {NULL, 0},
};


//  Constants to get address of in option declaration
static const int nc_echo_raw = NC_ECHO_RAW;
static const int nc_echo_ascii = NC_ECHO_ASCII;
static const int nc_echo_quoted = NC_ECHO_QUOTED;
static const int nc_echo_msgpack = NC_ECHO_MSGPACK;

struct nc_enum_item echo_formats[] = {
    {"no", NC_NO_ECHO},
    {"raw", NC_ECHO_RAW},
    {"ascii", NC_ECHO_ASCII},
    {"quoted", NC_ECHO_QUOTED},
    {"msgpack", NC_ECHO_MSGPACK},
    {NULL, 0},
};

//  Constants for conflict masks
#define NC_MASK_SOCK 1
#define NC_MASK_WRITEABLE 2
#define NC_MASK_READABLE 4
#define NC_MASK_SOCK_SUB 8
#define NC_MASK_DATA 16
#define NC_NO_PROVIDES 0
#define NC_NO_CONFLICTS 0
#define NC_NO_REQUIRES 0
#define NC_MASK_SOCK_WRITEABLE (NC_MASK_SOCK | NC_MASK_WRITEABLE)
#define NC_MASK_SOCK_READABLE (NC_MASK_SOCK | NC_MASK_READABLE)
#define NC_MASK_SOCK_READWRITE  (NC_MASK_SOCK_WRITEABLE|NC_MASK_SOCK_READABLE)

struct nc_option nc_options[] = {
    // Generic options
    {"verbose", 'v', NULL,
     NC_OPT_INCREMENT, offsetof(nc_options_t, verbose), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_NO_REQUIRES,
     "Generic", NULL, "Increase verbosity of the nanocat"},
    {"silent", 'q', NULL,
     NC_OPT_DECREMENT, offsetof(nc_options_t, verbose), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_NO_REQUIRES,
     "Generic", NULL, "Decrease verbosity of the nanocat"},
    {"help", 'h', NULL,
     NC_OPT_HELP, 0, NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_NO_REQUIRES,
     "Generic", NULL, "This help text"},

    // Socket types
    {"push", 'p', "nn_push",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_push,
     NC_MASK_SOCK_WRITEABLE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_PUSH socket type"},
    {"pull", 'P', "nn_pull",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_pull,
     NC_MASK_SOCK_READABLE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_PULL socket type"},
    {"pub", 'S', "nn_pub",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_pub,
     NC_MASK_SOCK_WRITEABLE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_PUB socket type"},
    {"sub", 's', "nn_sub",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_sub,
     NC_MASK_SOCK_READABLE|NC_MASK_SOCK_SUB, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_SUB socket type"},
    {"req", 'R', "nn_req",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_req,
     NC_MASK_SOCK_READWRITE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_REQ socket type"},
    {"rep", 'r', "nn_rep",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_rep,
     NC_MASK_SOCK_READWRITE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_REP socket type"},
    {"surveyor", 'U', "nn_surveyor",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_surveyor,
     NC_MASK_SOCK_READWRITE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_SURVEYOR socket type"},
    {"respondent", 'u', "nn_respondent",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_respondent,
     NC_MASK_SOCK_READWRITE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_RESPONDENT socket type"},

    // Socket Options
    {"bind", 'b' , NULL,
     NC_OPT_STRING_LIST, offsetof(nc_options_t, bind_addresses), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_NO_REQUIRES,
     "Socket Options", "ADDR", "Bind socket to the address ADDR"},
    {"connect", 'c' , NULL,
     NC_OPT_STRING_LIST, offsetof(nc_options_t, connect_addresses), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_NO_REQUIRES,
     "Socket Options", "ADDR", "Connect socket to the address ADDR"},
    {"recv-timeout", 't', NULL,
     NC_OPT_FLOAT, offsetof(nc_options_t, recv_timeout), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_READABLE,
     "Socket Options", "SEC", "Set timeout for receiving a message"},
    {"send-timeout", 'T', NULL,
     NC_OPT_FLOAT, offsetof(nc_options_t, send_timeout), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_WRITEABLE,
     "Socket Options", "SEC", "Set timeout for sending a message"},

    // Pattern-specific options
    {"subscribe", 0, NULL,
     NC_OPT_STRING_LIST, offsetof(nc_options_t, subscriptions), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_SOCK_SUB,
     "SUB Socket Options", "PREFIX", "Subscribe to the prefix PREFIX. "
        "Note: socket will be subscribed to everything (empty prefix) if "
        "no prefixes are specified on the command-line."},

    // Echo Options
    {"format", 'f', NULL,
     NC_OPT_ENUM, offsetof(nc_options_t, echo_format), &echo_formats,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_READABLE,
     "Echo Options", "FORMAT", "Use echo format FORMAT "
                               "(same as the options below)"},
    {"raw", 0, NULL,
     NC_OPT_SET_ENUM, offsetof(nc_options_t, echo_format), &nc_echo_raw,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_READABLE,
     "Echo Options", NULL, "Dump message as is "
                           "(Note: no delimiters are printed)"},
    {"ascii", 'L', NULL,
     NC_OPT_SET_ENUM, offsetof(nc_options_t, echo_format), &nc_echo_ascii,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_READABLE,
     "Echo Options", NULL, "Print ASCII part of message delimited by newline. "
                           "All non-ascii characters replaced by dot."},
    {"quoted", 'Q', NULL,
     NC_OPT_SET_ENUM, offsetof(nc_options_t, echo_format), &nc_echo_quoted,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_READABLE,
     "Echo Options", NULL, "Print each message on separate line in double "
                           "quotes with C-like character escaping"},
    {"msgpack", 0, NULL,
     NC_OPT_SET_ENUM, offsetof(nc_options_t, echo_format), &nc_echo_msgpack,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_READABLE,
     "Echo Options", NULL, "Print each message as msgpacked string (raw type)."
                           " This is useful for programmatic parsing."},

    // Input Options
    {"data", 'D', NULL,
     NC_OPT_BLOB, offsetof(nc_options_t, data_to_send), &echo_formats,
     NC_MASK_DATA, NC_MASK_DATA, NC_MASK_WRITEABLE,
     "Data Options", "DATA", "Send DATA to the socket and quit for "
     "PUB, PUSH, PAIR socket. Use DATA to reply for REP or RESPONDENT socket. "
     "Send DATA as request for REQ or SURVEYOR socket. "},
    {"file", 'F', NULL,
     NC_OPT_READ_FILE, offsetof(nc_options_t, data_to_send), &echo_formats,
     NC_MASK_DATA, NC_MASK_DATA, NC_MASK_WRITEABLE,
     "Data Options", "PATH", "Same as --data but get data from file PATH"},

    // Sentinel
    {NULL}
    };


struct nc_commandline nc_cli = {
    .short_description = "A command-line interface to nanomsg",
    .long_description = "",
    .options = nc_options,
    .required_options = NC_MASK_SOCK,
    };


void nc_assert_errno(int flag, char *description) {
    if(!flag) {
        int err = errno;
        fprintf(stderr, description);
        fprintf(stderr, ": %s\n", strerror(err));
        exit(3);
    }
}

void nc_sub_init(nc_options_t *options, int sock) {
    int i;
    int rc;

    if(options->subscriptions.num) {
        for(i = 0; i < options->subscriptions.num; ++i) {
            rc = nn_setsockopt(sock, NN_SUB, NN_SUB_SUBSCRIBE,
                options->subscriptions.items[i],
                strlen(options->subscriptions.items[i]));
            nc_assert_errno(rc == 0, "Can't subscribe");
        }
    } else {
        rc = nn_setsockopt(sock, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
        nc_assert_errno(rc == 0, "Can't subscribe");
    }
}

int nc_create_socket(nc_options_t *options) {
    int sock;
    int rc;
    int millis;

    sock = nn_socket(AF_SP, options->socket_type);
    nc_assert_errno(sock >= 0, "Can't create socket");

    // Generic initialization
    if(options->send_timeout >= 0) {
        millis = (int)(options->send_timeout * 1000);
        rc = nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO,
                           &millis, sizeof(millis));
        nc_assert_errno(rc == 0, "Can't set send timeout");
    }
    if(options->recv_timeout >= 0) {
        millis = (int)(options->recv_timeout * 1000);
        rc = nn_setsockopt(sock, NN_SOL_SOCKET, NN_RCVTIMEO,
                           &millis, sizeof(millis));
        nc_assert_errno(rc == 0, "Can't set recv timeout");
    }

    // Specific intitalization
    switch(options->socket_type) {
    case NN_SUB:
        nc_sub_init(options, sock);
        break;
    }

    return sock;
}

void nc_connect_socket(nc_options_t *options, int sock) {
    int i;
    int rc;

    for(i = 0; i < options->bind_addresses.num; ++i) {
        rc = nn_bind(sock, options->bind_addresses.items[i]);
        nc_assert_errno(rc >= 0, "Can't bind");
    }
    for(i = 0; i < options->connect_addresses.num; ++i) {
        rc = nn_connect(sock, options->connect_addresses.items[i]);
        nc_assert_errno(rc >= 0, "Can't connect");
    }
}

int main(int argc, char **argv) {
    int sock;
    nc_options_t options = {
        .verbose = 0,
        .socket_type = 0,
        .bind_addresses = {NULL, 0},
        .connect_addresses = {NULL, 0},
        .send_timeout = -1.f,
        .recv_timeout = -1.f,
        .subscriptions = {NULL, 0},
        .data_to_send = {NULL, 0},
        .echo_format = NC_NO_ECHO
        };

    nc_parse_options(&nc_cli, &options, argc, argv);
    sock = nc_create_socket(&options);
    nc_connect_socket(&options, sock);
//    nc_loop(&options, sock);
    nn_close(sock);
}
