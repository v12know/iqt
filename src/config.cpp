#include "config.h"

#include <fstream>
#include <iostream>

#define CEREAL_XML_STRING_VALUE "iqt"
#include "cereal/cereal.hpp"
#include "cereal/archives/xml.hpp"
#include "cereal/types/string.hpp"

//#include "base/log.h"
namespace cereal {

//Archive Specialization
template <class Archive, class C,
		traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void save( Archive & ar, std::vector<iqt::Redis, C> const & vec )
  {
    for( const auto & i : vec )
      ar( cereal::make_nvp( "redis", i ) );
  }

template <class Archive, class C,
		traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void load( Archive & ar, std::vector<iqt::Redis, C> & vec )
{
	vec.clear();

	while( true )
	{
		const auto namePtr = ar.getNodeName();

		if( !namePtr )
			break;

		std::string key = namePtr;
		iqt::Redis value; ar( value );
		vec.emplace_back( std::move( value ) );
	}
}

} /* cereal */

namespace iqt
{
using std::cout;
using std::endl;

template<class archive>
inline void Logger::serialize(archive & ar)
{
	ar(CEREAL_NVP(level));
	ar(CEREAL_NVP(logFile));
	ar(CEREAL_NVP(asyncSize));
	ar(CEREAL_NVP(consoleFlag));
}

template<class archive>
inline void MdInfo::serialize(archive & ar)
{
	ar(CEREAL_NVP(brokerId));
	ar(CEREAL_NVP(userId));
	ar(CEREAL_NVP(password));
	ar(CEREAL_NVP(frontAddr));
	//ar(CEREAL_NVP(instrumentIds));
}

template<class archive>
inline void TdInfo::serialize(archive & ar)
{
	ar(CEREAL_NVP(acnId));
	ar(CEREAL_NVP(brokerId));
	ar(CEREAL_NVP(userId));
	ar(CEREAL_NVP(password));
	ar(CEREAL_NVP(frontAddr));
	ar(CEREAL_NVP(pubResumeType));
	ar(CEREAL_NVP(privResumeType));
	ar(CEREAL_NVP(tdAmt));
	ar(CEREAL_NVP(validTimeSpans));
	ar(CEREAL_NVP(capitalUpperRate));
}

template<class archive>
inline void Redis::serialize(archive & ar)
{
	ar(CEREAL_NVP(redisId));
	ar(CEREAL_NVP(host));
	ar(CEREAL_NVP(port));
	ar(CEREAL_NVP(password));
	ar(CEREAL_NVP(dbIndex));
}


template<class Archive>
inline void Config::serialize(Archive & ar)
{
	ar(CEREAL_NVP(logger));
	ar(CEREAL_NVP(mdInfo));
	ar(CEREAL_NVP(tdInfo));
	ar(CEREAL_NVP(redises));
	ar(CEREAL_NVP(grids));
	ar(CEREAL_NVP(optionGrids));
}

void Config::load(const std::string &configFile)
{
	std::ifstream file(configFile);
	cereal::XMLInputArchive archive(file);

	Config *conf = Config::instance();
	conf->serialize(archive);

}

void Config::save(const std::string &configFile)
{
	std::ofstream file(configFile);
	cereal::XMLOutputArchive archive(file);
	Config::instance()->serialize(archive);
}


} /* iqt */ 
