#include "core/blueprint.hxx"
#include "core/topo.hxx"
#include "test/models/blueprints/blueprints.hxx"
#include "test/models/topos/topologies.hxx"
#include "../catch.hpp"

using namespace marina;

/*
 *    basic blueprint equality tests
 */

TEST_CASE("mars-is-mars", "[jsonification]")
{
  Blueprint m = mars();
  Blueprint m_ = mars();

  REQUIRE( m == m_ );
}

TEST_CASE("mars-clone-is-mars", "[jsonification]")
{
  Blueprint m = mars();
  Blueprint m_ = m.clone();

  REQUIRE( m == m_ );
}

TEST_CASE("mars-copy-is-mars", "[jsonification]")
{
  Blueprint m = mars();
  Blueprint m_ = m;

  REQUIRE( m == m_ );
}

TEST_CASE("mars-is-not-mars0", "[jsonification]")
{
  Blueprint m = mars(),
            m_ = mars();

  REQUIRE( m == m_ );

  m_.getComputer("cortana")
    .memory(47_gb);
  
  REQUIRE( m != m_ );
}

TEST_CASE("mars-is-not-mars1", "[jsonification]")
{
  Blueprint m = mars(),
            m_ = mars();

  REQUIRE( m == m_ );

  m_.computer("sven")
    .cores(2);
  
  REQUIRE( m != m_ );
}

TEST_CASE("mars-is-not-mars2", "[jsonification]")
{
  Blueprint m = mars(),
            m_ = mars();

  REQUIRE( m == m_ );

  m_.removeComputer("cortana");

  REQUIRE( m != m_ );

}

/*
 *    basic topology equality tests
 */

TEST_CASE("deter-is-deter", "[jsonification]")
{
  TestbedTopology t = deter2015(),
                  t_ = deter2015();

  REQUIRE( t == t_ );
}

TEST_CASE("deter-clone-is-deter", "[jsonification]")
{
  TestbedTopology t = deter2015(),
                  t_ = t.clone();

  REQUIRE( t == t_ );
}

TEST_CASE("deter-copy-is-deter", "[jsonification]")
{
  TestbedTopology t = deter2015(),
                  t_ = t;

  REQUIRE( t == t_ );
}

TEST_CASE("deter-is-not-deter0", "[jsonification]")
{
  TestbedTopology t = deter2015(),
                  t_ = deter2015();

  REQUIRE( t == t_ );

  t_.getSw("g2s2")
    .backplane(47_gbps);

  REQUIRE( t != t_ );
}

TEST_CASE("deter-is-not-deter1", "[jsonification]")
{
  TestbedTopology t = deter2015(),
                  t_ = deter2015();

  REQUIRE( t == t_ );

  t_.sw("supermuffin")
    .backplane(666_mbps);
  
  REQUIRE( t != t_ );
}

TEST_CASE("deter-is-not-deter2", "[jsonification]")
{
  TestbedTopology t = deter2015(),
                  t_ = deter2015();

  REQUIRE( t == t_ );

  t_.host("supertaco")
    .cores(666);
  
  REQUIRE( t != t_ );
}

TEST_CASE("deter-is-not-deter3", "[jsonification]")
{
  TestbedTopology t = deter2015(),
                  t_ = deter2015();

  REQUIRE( t == t_ );

  t_.host("supertaco")
    .cores(666);
  REQUIRE( t != t_ );

  t_.removeHost("supertaco");
  REQUIRE( t == t_ );
  
  t_.sw("supermuffin")
    .backplane(666_mbps);
  REQUIRE( t != t_ );

  t_.removeSw("supermuffin");
  REQUIRE( t == t_ );

  t_.removeHost("mc47");
  REQUIRE( t != t_ );
}


/*
 *    json round trip tests
 */

TEST_CASE("mars-round-trip", "[jsonification]")
{
  Blueprint m = mars();
  Json j = m.json();
  auto m_ = Blueprint::fromJson(j);

  REQUIRE( m == m_ );
}

TEST_CASE("deter-round-trip", "[jsonification]")
{
  TestbedTopology t = deter2015();
  Json j = t.json();
  auto t_ = TestbedTopology::fromJson(j);

  REQUIRE( t == t_ );
}
