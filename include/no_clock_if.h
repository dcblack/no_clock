#ifndef NONCLOCK_IF_H
#define NONCLOCK_IF_H

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

#include <systemc>

// This interface allows the use of no_clock with SystemC ports - see no_clock.h for more information

class no_clock_if
: public sc_core::sc_interface
{
  virtual void               set_frequency       ( double           frequency ) = 0;
  virtual void               set_period_time     ( sc_core::sc_time period    ) = 0;
  virtual void               set_offset_time     ( sc_core::sc_time offset    ) = 0;
  virtual void               set_duty_cycle      ( double           duty      ) = 0;
  virtual void               set_sample_time     ( sc_core::sc_time sample    ) = 0;
  virtual void               set_setedge_time    ( sc_core::sc_time setedge   ) = 0;
  virtual const char*        name                ( void ) const = 0;
  virtual sc_core::sc_time   period              ( unsigned int cycles = 0U ) const = 0;
  virtual double             duty                ( void ) const = 0;
  virtual double             frequency           ( void ) const = 0;
  // Special conveniences
  virtual unsigned int       cycles              ( void ) const = 0; // Number of clock cycles since start
  virtual unsigned int       frequency_changes   ( void ) const = 0; // Number of times frequency was changed
  // Calculate the delay till... (use for temporal offset)...may return SC_ZERO_TIME if already on the edge
  virtual sc_core::sc_time   until_posedge       ( unsigned int cycles = 0U ) const = 0;
  virtual sc_core::sc_time   until_negedge       ( unsigned int cycles = 0U ) const = 0;
  virtual sc_core::sc_time   until_anyedge       ( unsigned int cycles = 0U ) const = 0;
  virtual sc_core::sc_time   until_sample        ( unsigned int cycles = 0U ) const = 0;
  virtual sc_core::sc_time   until_setedge       ( unsigned int cycles = 0U ) const = 0;
  // Calculate the delay till... (use for temporal offset)...never returns SC_ZERO_TIME
  virtual sc_core::sc_time   next_posedge       ( unsigned int cycles = 0U ) const = 0;
  virtual sc_core::sc_time   next_negedge       ( unsigned int cycles = 0U ) const = 0;
  virtual sc_core::sc_time   next_anyedge       ( unsigned int cycles = 0U ) const = 0;
  virtual sc_core::sc_time   next_sample        ( unsigned int cycles = 0U ) const = 0;
  virtual sc_core::sc_time   next_setedge       ( unsigned int cycles = 0U ) const = 0;
  // Wait only if really necessary (for use in SC_THREAD)
  virtual void               wait_posedge        ( unsigned int cycles = 0U ) = 0;
  virtual void               wait_negedge        ( unsigned int cycles = 0U ) = 0;
  virtual void               wait_anyedge        ( unsigned int cycles = 0U ) = 0;
  virtual void               wait_sample         ( unsigned int cycles = 0U ) = 0;
  virtual void               wait_setedge        ( unsigned int cycles = 0U ) = 0;
  // Are we there? (use in SC_METHOD)
  virtual bool               at_posedge_time     ( void ) const = 0;
  virtual bool               posedge             ( void ) const = 0;
  virtual bool               at_negedge_time     ( void ) const = 0;
  virtual bool               negedge             ( void ) const = 0;
  virtual bool               at_anyedge_time     ( void ) const = 0;
  virtual bool               event               ( void ) const = 0;
  virtual bool               at_sample_time      ( void ) const = 0;
  virtual bool               at_setedge_time     ( void ) const = 0;
  // For compatibility if you really have/want to
  virtual sc_core::sc_event& default_event       ( size_t events = 0 ) = 0;
  virtual sc_core::sc_event& posedge_event       ( size_t events = 0 ) = 0;
  virtual sc_core::sc_event& negedge_event       ( size_t events = 0 ) = 0;
  virtual sc_core::sc_event& sample_event        ( size_t events = 0 ) = 0;
  virtual sc_core::sc_event& setedge_event       ( size_t events = 0 ) = 0;
  virtual sc_core::sc_event& value_changed_event ( size_t events = 0 ) = 0;
  virtual bool               read                ( void ) const = 0;
};
#endif
