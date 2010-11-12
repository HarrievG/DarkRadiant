#ifndef _PARTICLE_PARAMETER_H_
#define _PARTICLE_PARAMETER_H_

#include "iparticlestage.h"

#include "parser/DefTokeniser.h"
#include <boost/lexical_cast.hpp>

#include "itextstream.h"

namespace particles
{

/**
 * greebo: A particle parameter represents a bounded member value
 * of a particle stage (e.g. speed or size).
 *
 * Use the evaluate() method to retrieve a particular value.
 *
 * It is modeled after the idParticleParam class in the D3 SDK.
 */
class ParticleParameter :
	public IParticleParameter
{
private:
	float _from;	// lower bound
	float _to;		// upper bound

public:
	ParticleParameter() :
		_from(0),
		_to(0)
	{}

	ParticleParameter(float constantValue) :
		_from(constantValue),
		_to(constantValue)
	{}

	ParticleParameter(float from, float to) :
		_from(from),
		_to(to)
	{}

	float getFrom() const
	{
		return _from;
	}

	float getTo() const
	{
		return _to;
	}

	void setFrom(float value)
	{
		_from = value;
	}

	void setTo(float value)
	{
		_to = value;
	}

	float evaluate(float fraction) const
	{
		return _from + fraction * (_to - _from);
	}

	float integrate(float fraction) const
	{
		return (_to - _from) * 0.5f * (fraction*fraction) + _from * fraction;
	}

	void parseFromTokens(parser::DefTokeniser& tok)
	{
		std::string val = tok.nextToken();

		try
		{
			setFrom(boost::lexical_cast<float>(val));
		}
		catch (boost::bad_lexical_cast&)
		{
			globalErrorStream() << "[particles] Bad lower value, token is '" <<
				val << "'" << std::endl;
		}

		if (tok.peek() == "to")
		{
			// Upper value is there, parse it
			tok.skipTokens(1); // skip the "to"

			val = tok.nextToken();

			try
			{
				// cut off the quotes before converting to double
				setTo(boost::lexical_cast<float>(val));
			}
			catch (boost::bad_lexical_cast&)
			{
				globalErrorStream() << "[particles] Bad upper value, token is '" <<
					val << "'" << std::endl;
			}
		}
		else
		{
			setTo(getFrom());
		}
	}
};

} // namespace

#endif /* _PARTICLE_PARAMETER_H_ */
