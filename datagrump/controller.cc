#include <iostream>

#include "controller.hh"
#include "timestamp.hh"
#include <climits>
#include <cstdint>
#include <vector>
#include <algorithm>

using namespace std;

#define RTT_EXPAND_THRESH 40
#define RTT_CONTRACT_THRESH 70

/* Default constructor */
Controller::Controller( const bool debug)
  : debug_( debug ), rtt(50), old_rtt(50), max_rtt(40), min_rtt(40), wsz(50),
    recent_rtts()
{
  debug_ = false;
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{

  /* Establish a floor for wsz. */
  if (wsz < 5)
    wsz = 10;

  /* Default: fixed window size of 100 outstanding datagrams */
  unsigned int the_window_size = (unsigned int) wsz;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << " || with rtt: " << rtt << endl;
  }


  cerr << "__DEBUG__:       updating wsz:  " << wsz <<endl;
  return the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				                            /* of the sent datagram */
				                            const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
}

bool compare_int(const int a, const int b)
{
  return a < b;
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

  // if (rtt - old_rtt > 0) {
  //   wsz -= (((float)rtt - (float)old_rtt) / (float)old_rtt) * wsz;
  //   wsz = wsz < 0 ? 0 : wsz;
  //   if( debug_ ) cerr << "__DEBUG__: "<< "(" << wsz <<")"  << " decreasing: " << (((float)rtt - (float)old_rtt) / (float)old_rtt)  << endl;
  // } else {
  //   wsz += (((float)old_rtt - (float)rtt) / (float)rtt) * wsz;
  //   if (debug_) cerr << "__DEBUG__: "<< "(" << wsz <<")"  << " increasing: " << (((float)rtt - (float)old_rtt) / (float)old_rtt)  << endl;
  // }

  rtt = (timestamp_ack_received - send_timestamp_acked);

  recent_rtts.push_back(rtt);
  if (recent_rtts.size() >= 10000)
    recent_rtts.pop_front();

  min_rtt = INT_MAX;
  max_rtt = INT_MIN;

  vector<int> copy;

  for (int time : recent_rtts) 
    {
      copy.push_back(time);
    }

  sort(copy.begin(), copy.end(), compare_int);

  //int median = copy[copy.size() / 2];

  if (rtt <= copy[copy.size() / 4])//RTT_EXPAND_THRESH) 
    {
      wsz += 1;
      cerr << "__DEBUG__: expanding win " << wsz << " | rtt: " << rtt << endl;
    } 
  else if (rtt >= copy[3*copy.size() / 4])//RTT_CONTRACT_THRESH) 
    {
      wsz /= 2;
      cerr << "__DEBUG__: contracting win " << wsz << " | rtt: " << rtt << endl;
    }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 3*max_rtt;//(rtt / 2) + (rtt / 4) ; /* timeout of one second */
}
