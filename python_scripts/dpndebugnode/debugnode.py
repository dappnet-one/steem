import json, logging, sys

from datetime import datetime
from datetime import timezone
from os import devnull
from pathlib import Path
from signal import SIGINT, SIGTERM
from shutil import copy2, copytree
from subprocess import Popen
from tempfile import TemporaryDirectory
from threading import Lock
from time import sleep

from dpnapi.dpnnoderpc import DpnNodeRPC

class DebugNode( object ):
   """ Wraps the dpnd debug node plugin for easier automated testing of the Dpn Network"""

   def __init__( self, dpnd, data_dir, args='', plugins=[], apis=[], dpnd_out=None, dpnd_err=None ):
      """ Creates a dpnd debug node.

      It can be ran by using 'with debug_node:'
      While in the context of 'with' the debug node will continue to run.
      Upon exit of 'with' the debug will exit and clean up temporary files.
      This class also contains methods to allow basic manipulation of the blockchain.
      For all other requests, the python-dpn library should be used.

      args:
         dpnd -- The string path to the location of the dpnd binary
         data_dir -- The string path to an existing dpnd data directory which will be used to pull blocks from.
         args -- Other string args to pass to dpnd.
         plugins -- Any additional plugins to start with the debug node. Modify plugins DebugNode.plugins
         apis -- Any additional APIs to have available. APIs will retain this order for accesibility starting at id 3.
            database_api is 0, login_api is 1, and debug_node_api is 2. Modify apis with DebugNode.api
         dpnd_stdout -- A stream for dpnd's stdout. Default is to pipe to /dev/null
         dpnd_stderr -- A stream for dpnd's stderr. Default is to pipe to /dev/null
      """
      self._data_dir = None
      self._debug_key = None
      self._FNULL = None
      self._rpc = None
      self._dpnd_bin = None
      self._dpnd_lock = None
      self._dpnd_process = None
      self._temp_data_dir = None

      self._dpnd_bin = Path( dpnd )
      if( not self._dpnd_bin.exists() ):
         raise ValueError( 'dpnd does not exist' )
      if( not self._dpnd_bin.is_file() ):
         raise ValueError( 'dpnd is not a file' )

      self._data_dir = Path( data_dir )
      if( not self._data_dir.exists() ):
         raise ValueError( 'data_dir either does not exist or is not a properly constructed dpn data directory' )
      if( not self._data_dir.is_dir() ):
         raise ValueError( 'data_dir is not a directory' )

      self.plugins = plugins
      self.apis = apis

      if( args != '' ):
         self._args = args.split( "\\s" )
      else:
         self._args = list()

      self._FNULL = open( devnull, 'w' )
      if( dpnd_out != None ):
         self.dpnd_out = dpnd_out
      else:
         self.dpnd_out = self._FNULL

      if( dpnd_err != None ):
         self.dpnd_err = dpnd_err
      else:
         self.dpnd_err = self._FNULL

      self._debug_key = '5JHNbFNDg834SFj8CMArV6YW7td4zrPzXveqTfaShmYVuYNeK69'
      self._dpnd_lock = Lock()


   def __enter__( self ):
      self._dpnd_lock.acquire()

      # Setup temp directory to use as the data directory for this
      self._temp_data_dir = TemporaryDirectory()

      for child in self._data_dir.iterdir():
         if( child.is_dir() ):
            copytree( str( child ), str( self._temp_data_dir.name ) + '/' + child.name )

      db_version = Path( self._data_dir.name ) / 'db_version'
      if( db_version.exists() and not db_version.is_dir() ):
         copy2( str( db_version ), str( self._temp_data_dir.name ) + '/db_version' )

      config = Path( self._temp_data_dir.name ) / 'config.ini'
      config.touch()
      config.write_text( self._get_config() )

      dpnd = [ str( self._dpnd_bin ), '--data-dir=' + str( self._temp_data_dir.name ) ]
      dpnd.extend( self._args )

      self._dpnd_process = Popen( dpnd, stdout=self.dpnd_out, stderr=self.dpnd_err )
      self._dpnd_process.poll()
      sleep( 5 )
      if( not self._dpnd_process.returncode ):
         self._rpc = DpnNodeRPC( 'ws://127.0.0.1:8095', '', '' )
      else:
         raise Exception( "dpnd did not start properly..." )

   def __exit__( self, exc, value, tb ):
      self._rpc = None

      if( self._dpnd_process != None ):
         self._dpnd_process.poll()

         if( not self._dpnd_process.returncode ):
            self._dpnd_process.send_signal( SIGINT )

            sleep( 7 )
            self._dpnd_process.poll()

            if( not self._dpnd_process.returncode ):
               self._dpnd_process.send_signal( SIGTERM )

               sleep( 5 )
               self._dpnd_process.poll()

               if( self._dpnd_process.returncode ):
                  loggin.error( 'dpnd did not properly shut down after SIGINT and SIGTERM. User intervention may be required.' )

      self._dpnd_process = None
      self._temp_data_dir.cleanup()
      self._temp_data_dir = None
      self._dpnd_lock.release()


   def _get_config( self ):
      return "# no seed-node in config file or command line\n" \
          + "p2p-endpoint = 127.0.0.1:2001       # bind to localhost to prevent remote p2p nodes from connecting to us\n" \
          + "rpc-endpoint = 127.0.0.1:8095       # bind to localhost to secure RPC API access\n" \
          + "enable-plugin = witness debug_node " + " ".join( self.plugins ) + "\n" \
          + "public-api = database_api login_api debug_node_api " + " ".join( self.apis ) + "\n"


   def debug_generate_blocks( self, count ):
      """
      Generate blocks on the current chain. Pending transactions will be applied, otherwise the
      blocks will be empty.

      The debug node plugin requires a WIF key to sign blocks with. This class uses the key
      5JHNbFNDg834SFj8CMArV6YW7td4zrPzXveqTfaShmYVuYNeK69 which was generated from
      `get_dev_key dpn debug`. Do not use this key on the live chain for any reason.

      args:
         count -- The number of new blocks to generate.

      returns:
         int: The number of blocks actually pushed.
      """
      if( count < 0 ):
         raise ValueError( "count must be a positive non-zero number" )
      return self._rpc.rpcexec( json.loads( '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_generate_blocks",["' + self._debug_key + '",' + str( count ) + ']], "id": 1}' ) )


   def debug_generate_blocks_until( self, timestamp, generate_sparsely=True ):
      """
      Generate block up until a head block time rather than a specific number of blocks. As with
      `debug_generate_blocks` all blocks will be empty unless there were pending transactions.

      The debug node plugin requires a WIF key to sign blocks with. This class uses the key
      5JHNbFNDg834SFj8CMArV6YW7td4zrPzXveqTfaShmYVuYNeK69 which was generated from
      `get_dev_key dpn debug`. Do not use this key on the live chain for any reason.

      args:
         time -- The desired new head block time. This is a POSIX Timestmap.
         generate_sparsely -- True if you wish to skip all intermediate blocks between the current
            head block time and the desired head block time. This is useful to trigger events, such
            as payouts and bandwidth updates, without generating blocks. However, many automatic chain
            updates (such as block inflation) will not continue at their normal rate as they are only
            calculated when a block is produced.

      returns:
         (time, int): A tuple including the new head block time and the number of blocks that were
            generated.
      """
      if( not isinstance( timestamp, int ) ):
         raise ValueError( "Time must be a int" )
      generate_sparsely_str = "true"
      if( not generate_sparsely ):
         generate_sparsely_str = "false"

      iso_string = datetime.fromtimestamp( timestamp, timezone.utc ).isoformat().split( '+' )[0].split( '-' )
      if( len( iso_string ) == 4 ):
         iso_string = iso_string[:-1]
      iso_string = '-'.join( iso_string )

      print( iso_string )
      return self._rpc.rpcexec( json.loads( '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_generate_blocks_until",["' + self._debug_key + '","' + iso_string + '","' + generate_sparsely_str + '"]], "id": 1}' ) )


   def debug_set_hardfork( self, hardfork_id ):
      """
      Schedules a hardfork to happen on the next block. call `debug_generate_blocks( 1 )` to trigger
      the hardfork. All hardforks with id less than or equal to hardfork_id will be scheduled and
      triggered.

      args:
         hardfork_id: The id of the hardfork to set. Hardfork IDs start at 1 (0 is genesis) and increment
            by one for each hardfork. The maximum value is DPN_NUM_HARDFORKS in chain/hardfork.d/0-preamble.hf
      """
      if( hardfork_id < 0 ):
         raise ValueError( "hardfork_id cannot be negative" )

      self._rpc.rpcexec( json.loads( '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_set_hardfork",[' + str( hardfork_id ) + ']], "id":1}' ) )


   def debug_has_hardfork( self, hardfork_id ):
      return self._rpc.rpcexec( json.loads( '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_has_hardfork",[' + str( hardfork_id ) + ']], "id":1}' ) )


   def debug_get_witness_schedule( self ):
      return self._rpc.rpcexec( json.loads( '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_get_witness_schedule",[]], "id":1}' ) )


   def debug_get_hardfork_property_object( self ):
      return self._rpc.rpcexec( json.loads( '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_get_hardfork_property_object",[]], "id":1}' ) )


if __name__=="__main__":
   WAITING = True

   def main():
      global WAITING
      """
      This example contains a simple parser to obtain the locations of both dpnd and the data directory,
      creates and runs a new debug node, replays all of the blocks in the data directory, and finally waits
      for the user to interface with it outside of the script. Sending SIGINT succesfully and cleanly terminates
      the program.
      """
      import os, signal, sys
      from argparse import ArgumentParser

      if( os.name != "posix" ):
         print( "This script only works on POSIX systems" )
         return

      parser = ArgumentParser( description='Run a Debug Node on an existing chain. This simply replays all blocks ' + \
                                 'and then waits indefinitely to allow user interaction through RPC calls and ' + \
                                 'the CLI wallet' )
      parser.add_argument( '--dpnd', '-s', type=str, required=True, help='The location of a dpnd binary to run the debug node' )
      parser.add_argument( '--data-dir', '-d', type=str, required=True, help='The location of an existing data directory. ' + \
                           'The debug node will pull blocks from this directory when replaying the chain. The directory ' + \
                           'will not be changed.' )

      args = parser.parse_args()

      dpnd = Path( args.dpnd )
      if( not dpnd.exists() ):
         print( 'Error: dpnd does not exist.' )
         return

      dpnd = dpnd.resolve()
      if( not dpnd.is_file() ):
         print( 'Error: dpnd is not a file.' )
         return

      data_dir = Path( args.data_dir )
      if( not data_dir.exists() ):
         print( 'Error: data_dir does not exist or is not a properly constructed dpnd data directory' )

      data_dir = data_dir.resolve()
      if( not data_dir.is_dir() ):
         print( 'Error: data_dir is not a directory' )

      signal.signal( signal.SIGINT, sigint_handler )

      print( 'Creating and starting debug node' )
      debug_node = DebugNode( str( dpnd ), str( data_dir ), dpnd_err=sys.stderr )

      with debug_node:
         print( 'Done!' )
         print( 'Feel free to interact with this node via RPC calls for the cli wallet.' )
         print( 'To shutdown the node, send SIGINT with Ctrl + C to this script. It will shut down safely.' )

         while( WAITING ):
            sleep( 1 )

   def sigint_handler( signum, frame ):
      global WAITING
      WAITING = False
      sleep( 3 )
      sys.exit( 0 )

   main()