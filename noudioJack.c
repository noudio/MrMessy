/*  noudioJack.c
 *
 *  This simple client demonstrates the basic features of JACK
 *  as they would be used by many applications.
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <jack/jack.h>


jack_client_t *client;
jack_port_t   *in1_port;
jack_port_t   *in2_port;
jack_port_t   *out2_port;
jack_port_t   *out1_port;


float vol = 0.05;
int   lfr = 1;

int   noudioJackInitDone = 0;
int   noudioJackExitDone = 0;
int   noudioJackDoConnect = 0;

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define TABLE_SIZE   (200)
typedef struct
{
    float sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
}
paTestData;

paTestData data; // data for process, must be global now


void disconnectMe(void);
void noudioJackExit(void);
int  noudioJackInit(bool doConnect);


int            buf_frame_rate = 0;
int            buf_frame_sz   = 0;
const    int   buf_total_sz   = 2*65536;
volatile int   buf_wptr       = 0;
float          buf_in1[2*65536];
float          buf_in2[2*65536];

static void signal_handler(int sig)
{
    fprintf(stderr, "\n\nsignal received, exiting ...\n");
    exit(0); // calls noudioJackExit automatically
}

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client follows a simple rule: when the JACK transport is
 * running, copy the input port to the output.  When it stops, exit.
 */


void noudioJackGetBuf(float *out1, float *out2, int sz)
{
    // get 'most recent': sz samples
    int buf_rptr = (buf_total_sz + buf_wptr - sz) % buf_total_sz;

    // copy to output buffer
    if (buf_rptr+sz <= buf_total_sz) {
        memcpy(out1, &buf_in1[buf_rptr], sz*sizeof(jack_default_audio_sample_t));
        memcpy(out2, &buf_in2[buf_rptr], sz*sizeof(jack_default_audio_sample_t));
    }
    else {
        int spl_sz = buf_rptr+sz  -buf_total_sz;
        int spl_if = buf_total_sz -buf_rptr;

        memcpy(out1,          &buf_in1[buf_rptr], spl_if*sizeof(jack_default_audio_sample_t));
        memcpy(&out1[spl_if], buf_in1,            spl_sz*sizeof(jack_default_audio_sample_t));
        memcpy(out2,          &buf_in2[buf_rptr], spl_if*sizeof(jack_default_audio_sample_t));
        memcpy(&out2[spl_if], buf_in2,            spl_sz*sizeof(jack_default_audio_sample_t));

    }
}


int
process (jack_nframes_t nframes, void *arg)
{
    jack_default_audio_sample_t *in1, *in2;
    jack_default_audio_sample_t *out1, *out2;
    paTestData *data = (paTestData*)arg;
    int i;

    in1  = (jack_default_audio_sample_t*)jack_port_get_buffer (in1_port,  nframes);
    in2  = (jack_default_audio_sample_t*)jack_port_get_buffer (in2_port,  nframes);
    out1 = (jack_default_audio_sample_t*)jack_port_get_buffer (out1_port, nframes);
    out2 = (jack_default_audio_sample_t*)jack_port_get_buffer (out2_port, nframes);

    // copy to output
    memcpy(out1, in1, nframes*sizeof(jack_default_audio_sample_t));
    memcpy(out2, in2, nframes*sizeof(jack_default_audio_sample_t));

    // copy to our own ring buffer
    if (buf_wptr+nframes <= buf_total_sz) {
        memcpy(&buf_in1[buf_wptr], in1,         nframes*sizeof(jack_default_audio_sample_t));
        memcpy(&buf_in2[buf_wptr], in2,         nframes*sizeof(jack_default_audio_sample_t));
    }
    else {
        int spl_sz = buf_wptr+nframes-buf_total_sz;
        int spl_if = buf_total_sz-buf_wptr;

        memcpy(&buf_in1[buf_wptr], in1,          spl_if*sizeof(jack_default_audio_sample_t));
        memcpy(buf_in1,            &in1[spl_if], spl_sz*sizeof(jack_default_audio_sample_t));

        memcpy(&buf_in2[buf_wptr], in2,          spl_if*sizeof(jack_default_audio_sample_t));
        memcpy(buf_in2,            &in2[spl_if], spl_sz*sizeof(jack_default_audio_sample_t));
    }
    buf_wptr = (buf_wptr + nframes) % buf_total_sz;


    /*
    for( i=0; i<nframes; i++ )
    {
        out1[i] += vol*data->sine[data->left_phase];  // left
        out2[i] += vol*data->sine[data->right_phase];  // right
        data->left_phase += lfr;
        if( data->left_phase >= TABLE_SIZE ) data->left_phase -= TABLE_SIZE;
        data->right_phase += 3; // higher pitch so we can distinguish left and right.
        if( data->right_phase >= TABLE_SIZE ) data->right_phase -= TABLE_SIZE;
    }
    */

    return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown (void *arg)
{
    exit (1);
}

int
noudioJackInit(int doConnect)
{
    int argc = 1;
    const char *argv[] = {"noudioJack", (char*)0};

    const char **ports;
    const char *program_name;
    const char *client_name;
    const char *server_name = NULL;
    jack_options_t options = JackNullOption;
    jack_status_t status;
    int i;


    if (noudioJackInitDone)
        return 0;

    noudioJackDoConnect = doConnect;

    program_name = strrchr(argv[0], '/');
    if (program_name == 0) {
        program_name = argv[0];
    } else {
        program_name++;
    }

    client_name = program_name;

    if (argc >= 2) {                       /* client name specified? */

        if (!strcmp(argv[1], "-h") ||
            !strcmp(argv[1], "-help") ||
            strcmp(argv[1], "--help")) {
            fprintf(stderr, "Usage: %s [jack-client-name] [jack-server-name]\n", program_name);
            exit(1);
        }

        client_name = argv[1];
        if (argc >= 3) {                   /* server name specified? */
            server_name = argv[2];
            int my_option = JackNullOption | JackServerName;
            options = (jack_options_t)my_option;
        }
    }

    // --------------------------------------------------------------------------------
    for( i=0; i<TABLE_SIZE; i++ )
    {
        data.sine[i] = 0.01 * (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
    }
    data.left_phase = data.right_phase = 0;


    // --------------------------------------------------------------------------------
    // open a client connection to the JACK server

    client = jack_client_open (client_name, options, &status, server_name);
    if (client == NULL) {
        fprintf (stderr, "jack_client_open() failed, "
        "status = 0x%2.0x\n", status);
        if (status & JackServerFailed) {
            fprintf (stderr, "Unable to connect to JACK server\n");
        }
        exit (1);
    }
    if (status & JackServerStarted) {
        fprintf (stderr, "JACK server started\n");
    }
    if (status & JackNameNotUnique) {
        client_name = jack_get_client_name(client);
        fprintf (stderr, "unique name `%s' assigned\n", client_name);
    }

    // ================================
    // get info

    int buf_frame_sz   = jack_get_buffer_size(client);
    int buf_frame_rate = jack_get_sample_rate(client);
    fprintf(stderr, "frame_sz:%d, _rate:%d\n", buf_frame_sz, buf_frame_rate);

    // ==================================
    // initialize databuffers
    memset(buf_in1, 0, buf_total_sz*sizeof(jack_default_audio_sample_t));
    memset(buf_in2, 0, buf_total_sz*sizeof(jack_default_audio_sample_t));
    buf_wptr = 0;


    /* tell the JACK server to call `process()' whenever
     *           there is work to be done.
     */

    jack_set_process_callback (client, process, &data);

    /* tell the JACK server to call `jack_shutdown()' if
     *           it ever shuts down, either entirely, or if it
     *           just decides to stop calling us.
     */

    jack_on_shutdown (client, jack_shutdown, 0);

    /* create two input ports */

    in1_port = jack_port_register  (client, "in_1",
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsInput, 0);

    in2_port = jack_port_register  (client, "in_2",
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsInput, 0);

    if ((in1_port == NULL) || (in2_port == NULL)) {
        fprintf(stderr, "no more JACK ports available\n");
        exit (1);
    }
    /* create two output ports */

    out1_port = jack_port_register (client, "out_1",
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsOutput, 0);

    out2_port = jack_port_register (client, "out_2",
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsOutput, 0);

    if ((out1_port == NULL) || (out2_port == NULL)) {
        fprintf(stderr, "no more JACK ports available\n");
        exit (1);
    }

    /* Tell the JACK server that we are ready to roll.  Our
     * process() callback will start running now. */

    if (jack_activate (client)) {
        fprintf (stderr, "cannot activate client");
        exit (1);
    }

    /* Connect the ports.  You can't do this before the client is
     * activated, because we can't make connections to clients
     * that aren't running.  Note the confusing (but necessary)
     * orientation of the driver backend ports: playback ports are
     * "input" to the backend, and capture ports are "output" from
     * it.
     */

    if (noudioJackDoConnect) {

        ports = jack_get_ports (client, NULL, NULL,
                                JackPortIsPhysical|JackPortIsInput);
        if (ports == NULL) {
            fprintf(stderr, "no physical playback ports\n");
            exit (1);
        }

        if (jack_connect (client, jack_port_name (out1_port), ports[0])) {
            fprintf (stderr, "cannot connect output ports\n");
        }
        else {
            if (ports[0]) fprintf (stderr, "%s connected to: %s\n", jack_port_name (out1_port), ports[0]);
        }

        if (jack_connect (client, jack_port_name (out2_port), ports[1])) {
            fprintf (stderr, "cannot connect output ports\n");
        }
        else {
            if (ports[1]) fprintf (stderr, "%s connected to: %s\n", jack_port_name (out2_port), ports[1]);
        }

        // ===========================================================================
        // Get all feeders from port[0], and connect them to in[0]
        if (ports && ports[0]) {
            const char  **conns;
            const char  **con_iter;
            jack_port_t *thePort;

            thePort = jack_port_by_name(client, ports[0]);
            conns   = jack_port_get_all_connections(client, thePort);
            for (con_iter = conns; con_iter && *con_iter; con_iter++) {

                // if this is our own port, skip
                if (!strcmp(*con_iter, jack_port_name (out1_port)))
                    continue;

                // connect that port to our input
                if (jack_connect (client, *con_iter, jack_port_name (in1_port))) {
                    fprintf (stderr, "cannot connect input port\n");
                }
                else {
                    fprintf (stderr, "%s connected to: %s\n", *con_iter, jack_port_name (in1_port));
                }
                // disconnect that port from port[0]
                if (jack_disconnect (client, *con_iter, ports[0])) {
                    fprintf (stderr, "cannot disconnect output port\n");
                }
                else {
                    fprintf (stderr, "%s disconnected from: %s\n", *con_iter, ports[0]);
                }

            }
            if (conns) free(conns);
        }
        // ===========================================================================
        // Get all feeders from port[1], and connect them to in[1]
        if (ports && ports[1]){
            const char  **conns;
            const char  **con_iter;
            jack_port_t *thePort;

            thePort = jack_port_by_name(client, ports[1]);
            conns   = jack_port_get_all_connections(client, thePort);

            for (con_iter = conns; con_iter && *con_iter; con_iter++) {

                // if this is our own port, skip
                if (!strcmp(*con_iter, jack_port_name (out2_port)))
                    continue;

                // connect that port to our input
                if (jack_connect (client, *con_iter, jack_port_name (in2_port))) {
                    fprintf (stderr, "cannot connect input port\n");
                }
                else {
                    fprintf (stderr, "%s connected to: %s\n", *con_iter, jack_port_name (in2_port));
                }
                // disconnect that port from port[0]
                if (jack_disconnect (client, *con_iter, ports[1])) {
                    fprintf (stderr, "cannot disconnect output port\n");
                }
                else {
                    fprintf (stderr, "%s disconnected from: %s\n", *con_iter, ports[1]);
                }

            }
            if (conns) free(conns);
        }
        // =======================
        if (ports) free (ports);

    } // if (! noudioJackNoConnect)

    /* install a signal handler to properly quits jack client */
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP,  signal_handler);
    signal(SIGINT,  signal_handler);

    noudioJackInitDone = 1;
    noudioJackExitDone = 0;
    atexit(noudioJackExit);
    return 0;
}

// ======================================================================
void routeFromPort2Port(jack_port_t *iP, jack_port_t *oP)
{
    // route all inputs, to where op is connected to
    const char **iPlist;
    const char **iPiter;
    const char **oPlist;
    const char **oPiter;

    iPlist = jack_port_get_all_connections(client, iP);
    oPlist = jack_port_get_all_connections(client, oP);

    // for each oPList port
    //  1 first disconnect oP from it
    //  for each iPlist port
    //      connect to opList port
    //      disconnect from iP

    for (oPiter = oPlist; oPiter && *oPiter; oPiter++) {

        // disconnect oP
        if (jack_disconnect (client, jack_port_name (oP), *oPiter)) {
            fprintf (stderr, "cannot disconnect outport\n");
        }
        else {
            fprintf (stderr, "%s disconnected from: %s\n", jack_port_name (oP), *oPiter);
        }

        for (iPiter = iPlist; iPiter && *iPiter; iPiter++) {
            if (jack_connect (client, *iPiter, *oPiter)) {
                fprintf (stderr, "cannot connect inport\n");
            }
            else {
                fprintf (stderr, "%s connected to: %s\n", *iPiter, *oPiter);
            }
            // disconnect iP
            if (jack_disconnect (client, *iPiter, jack_port_name (iP))) {
                fprintf (stderr, "cannot disconnect inport\n");
            }
            else {
                fprintf (stderr, "%s disconnected from: %s\n", *iPiter, jack_port_name (iP));
            }
        }
    }
}

void disconnectMe(void)
{
    routeFromPort2Port(in1_port, out1_port);
    routeFromPort2Port(in2_port, out2_port);
}

void noudioJackExit(void)
{
    if (noudioJackExitDone)
        return;

    if (noudioJackDoConnect) {
        disconnectMe();
    }
    jack_client_close (client);
    noudioJackExitDone = 1;
    noudioJackInitDone = 0;
}


#ifdef TEST_MAIN
int main (int argc, char *argv[])
{
    noudioJackInit(false);
    /* keep running until the Ctrl+C */

    while (1) {
        char incommand[17];

        fprintf(stderr, "noudioJack > ");
        fflush(stderr);

        if (fscanf(stdin, "%16s", incommand)==1) {
            if (!strcmp(incommand, "quit"))
                break;
            if (!strcmp(incommand, "+")) {
                vol *= 1.258925412;
                continue;
            }
            if (!strcmp(incommand, "-")) {
                vol /= 1.258925412;
                continue;
            }
            if (!strcmp(incommand, "l+")) {
                lfr += 1;
                continue;
            }
        }
        sleep (1);
    }
    noudioJackExit();
}
#endif
