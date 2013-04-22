#ifndef NONCLOCK_H
#define NONCLOCK_H

///////////////////////////////////////////////////////////////////////////////
// $License: Apache 2.0 $
//
// This file is licensed under the Apache License, Version 2.0 (the "License").
// You may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

////////////////////////////////////////////////////////////////////////////////
//
// DESCRIPTION
//   The no_clock object simplifies high-level modeling of clocks without
//   incurring the context switching overhead of a real clock.  It does so by
//   providing a number of methods that calculate timing offsets and reducing
//   multiple clock delays to a single wait.  This contrasts with traditional
//   clocks that issues millions of unused cycles that slow down simulation due
//   to the overhead of context switching.
//
//   To simplify the usage model, no_clock is designed to have a similar syntax
//   to a real sc_clock. This should make it easier for designers to adopt. In
//   fact, you should be able to replace no_clock with sc_clock at any time as
//   the design is refined without much change to the underlying code.
//
//   Below are declarations and corresponding timing diagrams that may be useful
//   to understanding the design.  We also provide a number of utility methods
//   in the no_clock class as an aid to modeling.
//
//   [Ed: The #ifdef allows syntax coloring/checking (in vim) only. Never define NEVER.]
#ifdef NEVER /* DOCUMENTATION */
     const sc_core::sc_time(1.0,SC_NS) ns;
     no_clock CLK1("CLK1",/*period*/10*ns,/*duty*/0.5,/*offset*/0*ns,/*1stpos*/true ,/*smpl*/1*ns,/*chg*/5*ns);
     no_clock CLK2("CLK2",/*period*/12*ns,/*duty*/0.3,/*offset*/1*ns,/*1stpos*/false,/*smpl*/3*ns,/*chg*/6*ns);
#endif
//   Examples of above definitions impact to the virtual clocks they define.
//   |                                |                                    |
//   |       _0123456789_123456789_1  |       _123456789_123456789_123456  |
//   |        :____     :____     :_  |       _:        ___:        ___:   |
//   |  CLK1 _|    |____|    |____|   |  CLK2  |_______|   |_______|   |_  |
//   |        :    :    :    :    :   |        :  :  :     :  :  :     :   |
//   |  DATA  :s   c    :s   c    :   |        :  s  c     :  s  c     :   |
//   |        ::   :    ::   :    :   |        :  :  :     :  :  :     :   |
//   |  Time  0:   :   10:   :   20   |  Time  1  :  :    13  :  :    25   |
//   |         1   5    11  15        |           4  7       16 19         |
//   |                                |                                    |
//
//   In diagram, (s)ample and (c)hange are simply abbreviations.
//
//   Finally, we implement the concept of global clocks as a convenience. This
//   allows modeling without the burden of connecting clocks, which may be
//   deferred to implementation. Each instantiation should maintain local
//   pointers to the global clocks it uses.
//
//   Note that no_clock does not inherit from sc_object, and may therefor be
//   instantiated at anytime. This is unlike sc_core::sc_clock, which is a
//   proper piece of SystemC hardware.
//
////////////////////////////////////////////////////////////////////////////////

#include "no_clock_if.h"
#include <systemc>
#include <map>

////////////////////////////////////////////////////////////////////////////////
//
//  #    # #######  ###  #      ###  #######  ###  #####   ####                          
//  #    #    #      #   #       #      #      #   #      #    #                         
//  #    #    #      #   #       #      #      #   #      #                              
//  #    #    #      #   #       #      #      #   #####   ####                          
//  #    #    #      #   #       #      #      #   #           #                         
//  #    #    #      #   #       #      #      #   #      #    #                         
//   ####     #     ###  #####  ###     #     ###  #####   ####                          
//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// Calculate a time remainder (missing from sc_time)
inline sc_core::sc_time operator % ( const sc_core::sc_time& lhs, const sc_core::sc_time& rhs )
{
  double lhd(lhs.to_seconds());
  double rhd(rhs.to_seconds());
  double rslt = lhd - rhd*int(lhd/rhd);
  return sc_core::sc_time(rslt,sc_core::SC_SEC);
}

//------------------------------------------------------------------------------
// For consistency
inline sc_core::sc_time operator % ( const sc_core::sc_time& lhs, const double& rhs )
{
  double lhd(lhs.to_seconds());
  double rslt = lhd - rhs*int(lhd/rhs);
  return sc_core::sc_time(rslt,sc_core::SC_SEC);
}

// For safety
inline
sc_core::sc_time sc_core_time_diff
( sc_core::sc_time& lhs
, sc_core::sc_time& rhs
, char* file
, int   lnum
)
{
  if (lhs >= rhs) return (lhs - rhs);
#ifdef INVERT_NEGATIVE
  sc_core::sc_report_handler::report
    ( sc_core::SC_WARNING
    , "time_diff"
    , "Negative time calculation returning the negative difference"
    , file, lnum);
  return (rhs - lhs);
#else
  sc_core::sc_report_handler::report
    ( sc_core::SC_WARNING
    , "time_diff"
    , "Negative time calculation returning SC_ZERO_TIME"
    , file, lnum);
  return sc_core::SC_ZERO_TIME;
#endif
}

#define sc_time_diff(lhs,rhs) sc_core_time_diff(lhs,rhs,__FILE__,__LINE__)

//------------------------------------------------------------------------------
// Calculate the delay necessary to get to a particular time offset for a clock
// Example: 
//  wait(42,SC_NS); // from SC_ZERO_TIME
//  assert(delay(sc_time(10,SC_NS)) == sc_time(8,SC_NS));
//  assert(delay(sc_time(10,SC_NS),sc_time(3,SC_NS)) == sc_time(11,SC_NS));
//  assert(delay(sc_time(10,SC_NS),sc_time(1,SC_NS)) == sc_time(9,SC_NS));
//  Returns a value (0..tPERIOD-) + tOFFSET -- thus may be zero
inline sc_core::sc_time delay
( sc_core::sc_time tPERIOD                        // clock period
, sc_core::sc_time tOFFSET=sc_core::SC_ZERO_TIME  // how much beyond the clock edge
, sc_core::sc_time tSHIFT=sc_core::SC_ZERO_TIME   // temporal offset to assume
);

//------------------------------------------------------------------------------
// Compute the number of clocks since tZERO
inline unsigned long int clocks
( sc_core::sc_time tPERIOD
, sc_core::sc_time tZERO=sc_core::SC_ZERO_TIME
, sc_core::sc_time tSHIFT=sc_core::SC_ZERO_TIME
);

////////////////////////////////////////////////////////////////////////////////
//
//  #     #  ####           ####  #      ####   ####  #    #                      
//  ##    # #    #         #    # #     #    # #    # #   #                       
//  # #   # #    #         #      #     #    # #      #  #                        
//  #  #  # #    #         #      #     #    # #      ###                         
//  #   # # #    #         #      #     #    # #      #  #                        
//  #    ## #    #         #    # #     #    # #    # #   #                       
//  #     #  ####  #######  ####  #####  ####   ####  #    #                      
//
////////////////////////////////////////////////////////////////////////////////
#include "no_clock_if.h"
typedef sc_core::sc_time (*get_time_t)(void);
class no_clock
: public sc_core::sc_object
, public no_clock_if
{
public:

  no_clock //< Constructor
  ( const char*             clock_instance
  , const sc_core::sc_time& tPERIOD
  , double                  duty
  , const sc_core::sc_time& tOFFSET
  , const sc_core::sc_time& tSAMPLE
  , const sc_core::sc_time& tSETEDGE
  , bool                    positive
  );
  no_clock //< Constructor
  ( const char*             clock_instance
  , const sc_core::sc_time& tPERIOD
  , double                  duty     = 0.5
  , const sc_core::sc_time& tOFFSET  = sc_core::SC_ZERO_TIME
  , bool                    positive = true
  );

  // Use following to create clock and retrieve pointer
  static no_clock* global // Global clock accessor
  ( const char*      clock_name
  , sc_core::sc_time tPERIOD
  , double           duty     = 0.5
  , sc_core::sc_time tOFFSET  = sc_core::SC_ZERO_TIME
  , sc_core::sc_time tSAMPLE  = sc_core::SC_ZERO_TIME
  , sc_core::sc_time tSETEDGE = sc_core::SC_ZERO_TIME
  , bool             positive = true
  );
  // Use following to retrieve pointer
  static no_clock* global ( const char* clock_name);

  virtual ~no_clock(void) { }

  // Accessors
  void set_frequency          ( double           frequency );
  void set_period_time        ( sc_core::sc_time tPERIOD   );
  void set_offset_time        ( sc_core::sc_time tOFFSET   );
  void set_duty_cycle         ( double           duty      );
  void set_sample_time        ( sc_core::sc_time tSAMPLE   );
  void set_setedge_time       ( sc_core::sc_time tSETEDGE   );
  void set_time_shift         ( sc_core::sc_time tSHIFT    );
  const char*      name       ( void ) const;
  sc_core::sc_time period     ( unsigned int cycles = 1 ) const;
  double           duty       ( void ) const;
  double           frequency  ( void ) const;
  sc_core::sc_time time_shift ( void ) const { return m_tSHIFT; }
  // Special conveniences
  unsigned int     cycles ( void ) const; // Number of clock cycles since start
  void             reset  ( void ); // Clears count & frequency change base (not yet implemented -- specification issues remain)
  unsigned int     frequency_changes ( void ) const { return m_freq_count; } // Number of times frequency was changed
  // Calculate the delay till... (use for temporal offset)...may return SC_ZERO_TIME if already on the edge
  sc_core::sc_time  until_posedge ( unsigned int cycles = 0U ) const;
  sc_core::sc_time  until_negedge ( unsigned int cycles = 0U ) const;
  sc_core::sc_time  until_anyedge ( unsigned int cycles = 0U ) const;
  sc_core::sc_time  until_sample  ( unsigned int cycles = 0U ) const;
  sc_core::sc_time  until_setedge ( unsigned int cycles = 0U ) const;
  // Calculate the delay till... (use for temporal offset)...never returns SC_ZERO_TIME
  sc_core::sc_time  next_posedge ( unsigned int cycles = 0U ) const;
  sc_core::sc_time  next_negedge ( unsigned int cycles = 0U ) const;
  sc_core::sc_time  next_anyedge ( unsigned int cycles = 0U ) const;
  sc_core::sc_time  next_sample  ( unsigned int cycles = 0U ) const;
  sc_core::sc_time  next_setedge ( unsigned int cycles = 0U ) const;
  // Wait only if really necessary (for use in SC_THREAD)
  void wait_posedge ( unsigned int cycles = 0U );
  void wait_negedge ( unsigned int cycles = 0U );
  void wait_anyedge ( unsigned int cycles = 0U );
  void wait_sample  ( unsigned int cycles = 0U );
  void wait_setedge ( unsigned int cycles = 0U );
  // Are we there? (use in SC_METHOD)
  bool at_posedge_time ( void ) const;
  bool posedge         ( void ) const { return at_posedge_time(); }
  bool at_negedge_time ( void ) const;
  bool negedge         ( void ) const { return at_negedge_time(); }
  bool at_anyedge_time ( void ) const;
  bool event           ( void ) const { return at_anyedge_time(); }
  bool at_sample_time  ( void ) const;
  bool at_setedge_time ( void ) const;
  // For compatibility if you really have/want to (with an extra feature)
  // - specify N > 0 to delay further out
  sc_core::sc_event& default_event       ( size_t events = 0 );
  sc_core::sc_event& posedge_event       ( size_t events = 0 );
  sc_core::sc_event& negedge_event       ( size_t events = 0 );
  sc_core::sc_event& sample_event        ( size_t events = 0 );
  sc_core::sc_event& setedge_event       ( size_t events = 0 );
  sc_core::sc_event& value_changed_event ( size_t events = 0 );
  bool      read                ( void ) const;

  virtual void write( const bool& ) { SC_REPORT_ERROR("/no_clock","write() not allowed on clock"); }

private:
  // Don't allow copying
  no_clock(no_clock& disallowed) {} // Copy constructor
  no_clock& operator= (no_clock& disallowed) {return disallowed;} // Assignment
  // Internal data
  get_time_t          m_get_time;// callback that returns current time
  const char *        m_clock_name;// clock name
  sc_core::sc_time    m_tPERIOD; // clock period
  double              m_duty;    // clock duty cycle (fraction high)
  sc_core::sc_time    m_tOFFSET; // offset to first edge
  sc_core::sc_time    m_tPOSEDGE;// offset to positive rising edge (calculated)
  sc_core::sc_time    m_tNEGEDGE;// offset to negative falling edge (calculated)
  bool                m_posedge; // start on posedge
  sc_core::sc_time    m_tSAMPLE; // when to read (usually tOFFSET+tHOLD)
  sc_core::sc_time    m_tSETEDGE; // when to write (usually after tSAMPLE, but at least tPERIOD-tSETUP)
  sc_core::sc_event   m_anyedge_event;
  sc_core::sc_event   m_posedge_event;
  sc_core::sc_event   m_negedge_event;
  sc_core::sc_event   m_sample_event;
  sc_core::sc_event   m_setedge_event;
  sc_core::sc_time    m_frequency_set; // time when period was last changed
  unsigned long int   m_freq_count;    // counts how many times frequency was changed
  unsigned long int   m_base_count;    // cycles up to last frequency change
  sc_core::sc_time    m_tSHIFT;  // temporal shift
  typedef std::map<const char*,no_clock*> clock_map_t;
  static clock_map_t  s_global;
};

////////////////////////////////////////////////////////////////////////////////
//
//  ### #     # #     ### #     # #####                                           
//   #  ##    # #      #  ##    # #                                               
//   #  # #   # #      #  # #   # #                                               
//   #  #  #  # #      #  #  #  # #####                                           
//   #  #   # # #      #  #   # # #                                               
//   #  #    ## #      #  #    ## #                                               
//  ### #     # ##### ### #     # #####                                           
//
////////////////////////////////////////////////////////////////////////////////
// For efficiency

//------------------------------------------------------------------------------
inline void no_clock::set_time_shift (sc_core::sc_time tSHIFT) { m_tSHIFT = tSHIFT; }

//------------------------------------------------------------------------------
inline sc_core::sc_time delay
( sc_core::sc_time tPERIOD // clock period
, sc_core::sc_time tOFFSET // beyond clock edge
, sc_core::sc_time tSHIFT  // temporal offset to assume
)
{
  sc_core::sc_time tREMAINDER = (sc_core::sc_time_stamp() + tSHIFT) % tPERIOD;
  if      ( tREMAINDER == tOFFSET ) return sc_core::SC_ZERO_TIME;
  else if ( tREMAINDER <  tOFFSET ) return tOFFSET - tREMAINDER;
  else                              return tPERIOD + tOFFSET - tREMAINDER;
}

//------------------------------------------------------------------------------
// Compute the number of clocks since tZERO
inline unsigned long int clocks
( sc_core::sc_time tPERIOD
, sc_core::sc_time tZERO
, sc_core::sc_time tSHIFT
)
{
  // TODO: Factor in frequency changes
  return (unsigned long int)(( sc_core::sc_time_stamp() + tSHIFT - tZERO ) / tPERIOD);
}

//------------------------------------------------------------------------------
// Accessors
//------------------------------------------------------------------------------
inline const char*      no_clock::name      ( void ) const { return m_clock_name; }
inline sc_core::sc_time no_clock::period    ( unsigned int cycles ) const { return cycles*m_tPERIOD; }
inline double           no_clock::duty      ( void ) const { return m_duty; }
inline double           no_clock::frequency ( void ) const { return sc_core::sc_time(1,sc_core::SC_SEC)/m_tPERIOD; }

//------------------------------------------------------------------------------
// Special conveniences
//------------------------------------------------------------------------------
inline unsigned int     no_clock::cycles ( void ) const // Number of clock cycles since start
{
  return m_base_count + clocks(m_tPERIOD,m_frequency_set,m_tSHIFT);
}

// Calculate the delay till... (use for temporal offset)
inline sc_core::sc_time  no_clock::until_posedge ( unsigned int cycles ) const
{
  return cycles*m_tPERIOD+delay(m_tPERIOD,m_tPOSEDGE,m_tSHIFT);
}

inline sc_core::sc_time  no_clock::until_negedge ( unsigned int cycles ) const
{
  return cycles*m_tPERIOD+delay(m_tPERIOD,m_tNEGEDGE,m_tSHIFT);
}

inline sc_core::sc_time  no_clock::until_anyedge ( unsigned int cycles ) const
{
  return cycles*m_tPERIOD+(true==read()?until_negedge():until_posedge());
}

inline sc_core::sc_time  no_clock::until_sample  ( unsigned int cycles ) const
{
  return cycles*m_tPERIOD+delay(m_tPERIOD,m_tSAMPLE,m_tSHIFT);
}

inline sc_core::sc_time  no_clock::until_setedge ( unsigned int cycles ) const
{
  return cycles*m_tPERIOD+delay(m_tPERIOD,m_tSETEDGE,m_tSHIFT);
}

// Calculate the delay till next... (use for temporal offset) - never returns 0
inline sc_core::sc_time  no_clock::next_posedge ( unsigned int cycles ) const
{
  sc_core::sc_time t(delay(m_tPERIOD,m_tPOSEDGE,m_tSHIFT));
  return (cycles + (sc_core::SC_ZERO_TIME == t?1:0)) * m_tPERIOD + t;
}

inline sc_core::sc_time  no_clock::next_negedge ( unsigned int cycles ) const
{
  sc_core::sc_time t(delay(m_tPERIOD,m_tNEGEDGE,m_tSHIFT));
  return (cycles + (sc_core::SC_ZERO_TIME == t?1:0)) * m_tPERIOD + t;
}

inline sc_core::sc_time  no_clock::next_anyedge ( unsigned int cycles ) const
{
  return cycles*m_tPERIOD+(true==read()?next_negedge():next_posedge());
}

inline sc_core::sc_time  no_clock::next_sample  ( unsigned int cycles ) const
{
  sc_core::sc_time t(delay(m_tPERIOD,m_tSAMPLE,m_tSHIFT));
  return (cycles + (sc_core::SC_ZERO_TIME == t?1:0)) * m_tPERIOD + t;
}

inline sc_core::sc_time  no_clock::next_setedge ( unsigned int cycles ) const
{
  sc_core::sc_time t(delay(m_tPERIOD,m_tSETEDGE,m_tSHIFT));
  return (cycles + (sc_core::SC_ZERO_TIME == t?1:0)) * m_tPERIOD + t;
}

// Wait only if really necessary (for use in SC_THREAD) -- may be a NOP if cycles == 0
inline void no_clock::wait_posedge ( unsigned int cycles )
{
  sc_core::sc_time t(until_posedge(cycles));
  if (sc_core::SC_ZERO_TIME != t) wait(t);
}

inline void no_clock::wait_negedge ( unsigned int cycles )
{
  sc_core::sc_time t(until_negedge(cycles));
  if (sc_core::SC_ZERO_TIME != t) wait(t);
}

inline void no_clock::wait_anyedge ( unsigned int cycles )
{
  sc_core::sc_time t(until_anyedge(cycles));
  if (sc_core::SC_ZERO_TIME != t) wait(t);
}

inline void no_clock::wait_sample  ( unsigned int cycles )
{
  sc_core::sc_time t(until_sample(cycles));
  if (sc_core::SC_ZERO_TIME != t) wait(t);
}

inline void no_clock::wait_setedge ( unsigned int cycles )
{
  sc_core::sc_time t(until_setedge(cycles));
  if (sc_core::SC_ZERO_TIME != t) wait(t);
}

// Are we there? (use in SC_METHOD)
inline bool no_clock::at_posedge_time ( void ) const
{
  return until_posedge(0) == sc_core::SC_ZERO_TIME;
}

inline bool no_clock::at_negedge_time ( void ) const
{
  return until_negedge(0) == sc_core::SC_ZERO_TIME;
}

inline bool no_clock::at_anyedge_time ( void ) const
{
  return until_anyedge(0) == sc_core::SC_ZERO_TIME;
}

inline bool no_clock::at_sample_time  ( void ) const
{
  return until_sample(0) == sc_core::SC_ZERO_TIME;
}

inline bool no_clock::at_setedge_time ( void ) const
{
  return until_sample(0) == sc_core::SC_ZERO_TIME;
}

// For compatibility if you really have/want to
inline sc_core::sc_event& no_clock::default_event       ( size_t events )
{
  return value_changed_event(events);
}

inline sc_core::sc_event& no_clock::posedge_event       ( size_t events )
{
  wait_posedge(events);
  m_posedge_event.notify(sc_core::SC_ZERO_TIME);
  return m_posedge_event;
}

inline sc_core::sc_event& no_clock::negedge_event       ( size_t events )
{
  wait_negedge(events);
  m_negedge_event.notify(sc_core::SC_ZERO_TIME);
  return m_negedge_event;
}

inline sc_core::sc_event& no_clock::sample_event        ( size_t events )
{
  wait_sample(events);
  m_sample_event.notify(sc_core::SC_ZERO_TIME);
  return m_sample_event;
}

inline sc_core::sc_event& no_clock::setedge_event       ( size_t events )
{
  wait_setedge(events);
  m_setedge_event.notify(sc_core::SC_ZERO_TIME);
  return m_setedge_event;
}

inline sc_core::sc_event& no_clock::value_changed_event ( size_t events )
{
  wait_anyedge(events);
  m_anyedge_event.notify(sc_core::SC_ZERO_TIME);
  return m_anyedge_event;
}

inline bool      no_clock::read                ( void ) const
{
  return until_negedge()<until_posedge();
}

#endif

// TAF!
