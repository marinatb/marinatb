#include "embed.hxx"
#include "topo.hxx"
#include "3p/pipes/pipes.hxx"
#include <stdexcept>

using std::vector;
using std::sort;
using std::make_pair;
using std::runtime_error;
using namespace marina;
using namespace pipes;

/*----------------------------------------------------------------------------*|
 * @SwitchEmbeddingScore
 * =====================
 *  This struct contains information that speaks to how well a @Blueprint is
 *  embedded within the @Computer objects connected to a particular @Switch.
 *----------------------------------------------------------------------------*/
struct SwitchEmbeddingScore
{
  SwitchEmbeddingScore(Switch sw) : sw{sw} {}

  //the switch being scored
  Switch sw;

  //# of hosts the nodes attached to this switch can support
  size_t hosting_capacity{0}; 

  //average load of all nodes attached to this switch before & after embedding
  double initial_load{0},
         embedded_load{0};
};

/* basis of comparison for @SwitchEmbeddingScores
 * ---------------------------------------------
 *  switches are compared through a second order comparison as follows
 *    1) hosting capacity
 *    2) load after embedding
 */
bool operator < (SwitchEmbeddingScore x, SwitchEmbeddingScore y)
{
  if(x.hosting_capacity >  y.hosting_capacity) return true;
  if(x.hosting_capacity == y.hosting_capacity)
    return x.embedded_load < y.embedded_load;
  return false;
}

size_t embedBlueprintOnSwitch(Switch s, Blueprint m)
{
  vector<Computer> xs = 
  m.computers()
    //don't care about computers that have already been embedded
    | filter([](auto x){ return x.embedding().assigned; })
    //sort the computers so we insert the 'heaviest weight' ones first
    | sort([](auto x, auto y){ return x.hwspec().norm() < y.hwspec().norm(); });

  //std::cout << __func__ << " tyring to embed " << xs.size() << " nodes" 
  //  << std::endl;

  size_t embedded{0};
  while(!xs.empty())
  {
    //grab the next computer to embed
    auto c = xs.back();
    
    auto candidates = 
    s.connectedHosts() 
      //augment each candidate host with the load it would have #c were embedded
      | map([c](auto x){ return make_pair(x, x.loadv() + c.hwspec()); })
      //remove overloaded nodes
      | filter([](auto x){ return x.second.overloaded(); })
      //assign to least loaded nodes first
      | sort([](auto x, auto y) { return x.second.norm() > y.second.norm(); })
      ; 

    //we have run out of usable nodes on this switch, break out of embedding
    if(candidates.empty()) break;

    //we found a home for c, so remove it from the embedding list
    xs.pop_back(); 
    //put c in it's new home
    candidates.back().first.experimentMachines().push_back(c);
    c.embedding( Embedding{candidates.back().first.name(), true} );
    //book-keeping
    embedded++;
  }
  return embedded;
}

/* scoreSwitch(Switch s, Blueprint m)
 * ------------------------------
 *  A simple algorithm for producing a @SwitchEmbeddingScore for a @Blueprint 
 *  m on a @Switch s
 */
SwitchEmbeddingScore scoreSwitch(Switch ss, Blueprint mm)
{
  //the embedding modifies the state of #s, this is just a scoring run
  //so we modify the state of a clone and leave the passed in one alone
  Switch s = ss.clone();
  Blueprint m = mm.clone();
  SwitchEmbeddingScore score{s};

  //compute aggreate load of the computers connected to the target switch
  auto calc_load = [&s]()
  {
    auto agg_load = 
    s.connectedHosts() 
      | map([](auto h){ return h.loadv(); }) | reduce(plus);

    return agg_load.norm();
  };

  //calculate the inital aggreate load before embedding
  score.initial_load = calc_load();
  
  //perform the embedding
  score.hosting_capacity = embedBlueprintOnSwitch(s, m);

  //calculate the post embedding aggregate load
  score.embedded_load = calc_load();

  return score;
}

void realizeEmbedding(Switch s, Blueprint m, TestbedTopology t)
{
  for(auto h : s.connectedHosts())
  {
    auto hh = *t.hosts().find(h.name());
    for(auto c : h.experimentMachines())
    {
      auto cc = m.getComputer(c.name());
      cc.embedding({h.name(), true});
      hh.experimentMachines().push_back(cc);
    }
  }
  auto ss = *t.switches().find(s.name());
  ss.connectedHosts() = s.connectedHosts();
}

TestbedTopology marina::embed(Blueprint m, TestbedTopology tt)
{
  TestbedTopology t = tt.clone();

  auto remaining = [](Blueprint m)
  {
    return (
      m.computers()
        | filter([](auto x){ return x.embedding().assigned; })
    ).size();
  };

  size_t round{0}, rr{0};
  while((rr = remaining(m)) > 0 && round <= 10 > 0)
  {
    std::cout 
      << "round-" << round++ 
      << " " << rr << " remaining"
      << std::endl;
    //score and sort the switches, see scoreSwitch(...) & 
    //struct SwitchEmbeddingScore for details
    auto switch_scores =
    t.switches()
      | map<vector>([&m](auto x){ return scoreSwitch(x, m); })
      | sort();


    auto x = switch_scores.front();

    if(x.hosting_capacity == 0)
    {
      throw runtime_error{"failed to embed"};
    }

    //realize the embedding of the top scoring switch
    std::cout 
      << "embedding -- "
      << x.sw.name() << "[" << x.sw.connectedHosts().size() <<  "]: " 
      << x.hosting_capacity << " "
      << std::setprecision(3) << x.initial_load 
      << " " << x.embedded_load
      << std::endl;

    realizeEmbedding(x.sw, m, t);
  }

  /*
  for(auto x : switch_scores)
  {
    std::cout 
      << x.sw.name() << "[" << x.sw.connectedHosts().size() <<  "]: " 
      << x.hosting_capacity << " "
      << std::setprecision(3) << x.initial_load 
      << " " << x.embedded_load
      << std::endl;
  }
  */

  return t;
}
