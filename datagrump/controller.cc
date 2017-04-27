#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

#define RTT_EXPAND_THRESH 40
#define RTT_CONTRACT_THRESH 200

/* Default constructor */
Controller::Controller( const bool debug)
  : debug_( debug ), rtt(50), old_rtt(50), last_sent(timestamp_ms()), wsz(50)
{
  debug_ = false;
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  unsigned int the_window_size = (unsigned int) wsz;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << " || with rtt: " << rtt << endl;
  }

  return the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				                            /* of the sent datagram */
				                            const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			                         /* what sequence number was acknowledged */
			                         const uint64_t send_timestamp_acked,
			                         /* when the acknowledged datagram was sent (sender's clock) */
			                         const uint64_t recv_timestamp_acked,
			                         /* when the acknowledged datagram was received (receiver's clock)*/
			                         const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  /* Default: take no action */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }

  rtt = (timestamp_ack_received - send_timestamp_acked);
  if (rtt < RTT_EXPAND_THRESH) 
    wsz += 1;
  else if (rtt > RTT_CONTRACT_THRESH) 
    wsz -= 1;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
