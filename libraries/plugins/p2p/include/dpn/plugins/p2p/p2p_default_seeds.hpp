#pragma once

#include <vector>

namespace dpn{ namespace plugins { namespace p2p {

#ifdef IS_TEST_NET
const std::vector< std::string > default_seeds;
#else
const std::vector< std::string > default_seeds = {
   "seed-east.dpnit.com:2001",          // dpnit
   "seed-central.dpnit.com:2001",       // dpnit
   "seed-west.dpnit.com:2001",          // dpnit
   "52.74.152.79:2001",                   // smooth.witness
   "anyx.io:2001",                        // anyx
   "seed.liondani.com:2016",              // liondani
   "gtg.dpn.house:2001",                // gtg
   "seed.jesta.us:2001",                  // jesta
   "lafonadpn.com:2001",                // lafona
   "dpn-seed.altcap.io:40696",          // ihashfury
   "seed.roelandp.nl:2001",               // roelandp
   "seed.timcliff.com:2001",              // timcliff
   "dpnseed.clayop.com:2001",           // clayop
   "seed.dpnviz.com:2001",              // ausbitbank
   "dpn-seed.lukestokes.info:2001",     // lukestokes
   "seed.dpnian.info:2001",             // drakos
   "seed.followbtcnews.com:2001",         // followbtcnews
   "node.mahdiyari.info:2001",            // mahdiyari
   "seed.riverdpn.com:2001",            // riverhead
   "seed1.blockbrothers.io:2001",         // blockbrothers
   "dpnseed-fin.privex.io:2001",        // privex
   "seed.yabapmatt.com:2001"              // yabapmatt
};
#endif

} } } // dpn::plugins::p2p
