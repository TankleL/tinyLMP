/* ----------------------------------------------------------------------------
MIT License

Copyright (c) 2018 廖添(Tankle L.)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
---------------------------------------------------------------------------- */

#pragma once

#include <vector>
#include <string>
#include <map>

/**
 * Dependency: tinyfsm
 * link: https://github.com/digint/tinyfsm.git
 */ 
#include "tinyfsm.hpp"		

namespace tinylmp
{
	/************************************************************************/
	/*                           FORWARD-DECLARATION                        */
	/************************************************************************/
	class Parser;

	/************************************************************************/
	/*                                 NODE                                 */
	/************************************************************************/
	struct Node
	{
		void reset()
		{
			m_attr.clear();
			m_name.clear();
			m_text.clear();
		}

		std::map<std::string, std::string>	m_attr;
		std::string							m_name;
		std::string							m_text;
	};

	/************************************************************************/
	/*                               DOCUMENT                               */
	/************************************************************************/
	class Document
	{
	public:
		void push_back(
			  const Node& node)
		{
			m_nodes.push_back(node);
		}

		const Node& node_at(
			size_t index) const
		{
			return m_nodes.at(index);
		}

		size_t node_count() const
		{
			return m_nodes.size();
		}

	protected:
		std::vector<Node>		m_nodes;
	};

	namespace _internal_ns
	{
		inline bool is_ascii(char ch)
		{
			return ((unsigned char)ch) <= 0x7f;
		}

		/************************************************************************/
		/*                             PARSE-EVENT                              */
		/************************************************************************/
		struct PrsEvent : tinyfsm::Event
		{
			PrsEvent(char in_ch)
				: ch(in_ch)
			{}

			char ch;
		};

		/************************************************************************/
		/*                             PARSE-STATE                              */
		/************************************************************************/
		class ParseState : public tinyfsm::MooreMachine<ParseState>
		{
		public:
			ParseState() {}
			virtual ~ParseState() {}

		public:
			virtual void react(PrsEvent const &) = 0;

		protected:
#define _DECL_PS_STATIC_MEMBER(_type, _name)	_type& _name()
			_DECL_PS_STATIC_MEMBER(std::string, gtext);
			_DECL_PS_STATIC_MEMBER(Document, gdoc);
			_DECL_PS_STATIC_MEMBER(Node, gndbody);
#undef _DECL_PS_STATIC_MEMBER
		};

		/************************************************************************/
		/*                           FORWARD-DECLARATION                        */
		/************************************************************************/
		class PS_Text;
		class PS_NDBody;

		/************************************************************************/
		/*                               PS-TEXT                                */
		/************************************************************************/
		class PS_Text : public ParseState
		{
		public:
			void react(PrsEvent const &ev) override
			{
				switch (ev.ch)
				{
				case '<':
					if (gtext().length() > 0)
					{ // convert cached text into a text node.
						Node nd;
						nd.m_text = gtext();
						gdoc().push_back(nd);
					}

					gtext().clear();
					gtext().push_back(ev.ch);
					transit<PS_NDBody>();
					break;

				default:
					gtext().push_back(ev.ch);
				}
			}
		};

		/************************************************************************/
		/*                               PS-NDBODY                              */
		/************************************************************************/
		class PS_NDBodyText;
		class PS_NDBodyWaitClose;
		class PS_NDBody : public ParseState
		{
		public:
			void entry() override
			{
				gndbody().reset();
			}

			void react(PrsEvent const &ev) override
			{
				if (!is_ascii(ev.ch) ||
					(ev.ch >= 'a' && ev.ch <= 'z') ||
					(ev.ch >= 'A' && ev.ch <= 'Z') ||
					ev.ch == '-' ||
					ev.ch == '_')
				{
					gtext().push_back(ev.ch);
					gndbody().m_name.push_back(ev.ch);
				}
				else if(ev.ch == '/')
				{
					transit<PS_NDBodyWaitClose>();
				}
				else if (ev.ch == '>')
				{
					transit<PS_NDBodyText>();
				}
				else
				{ // bad syntax
					gndbody().reset();

					transit<PS_Text>();
				}
			}
		};

		class PS_NDBodyWaitClose : public ParseState
		{
		public:
			void react(PrsEvent const &ev) override
			{
				if (ev.ch == '>')
				{
					gdoc().push_back(gndbody());
					gtext().clear();
					transit<PS_Text>();
				}
				else
				{ // bad syntax
					gtext().push_back(ev.ch);
					transit<PS_Text>();
				}
			}
		};

		class PS_NDBodyRemoteClose : public ParseState
		{
		public:
			void entry() override
			{
				m_name_chk.clear();
			}

			void react(PrsEvent const &ev) override
			{
				if (!is_ascii(ev.ch) ||
					(ev.ch >= 'a' && ev.ch <= 'z') ||
					(ev.ch >= 'A' && ev.ch <= 'Z') ||
					ev.ch == '-' ||
					ev.ch == '_')
				{
					m_name_chk.push_back(ev.ch);
				}
				if (ev.ch == '>')
				{
					if (m_name_chk == gndbody().m_name)
					{
						gdoc().push_back(gndbody());
						gtext().clear();
						transit<PS_Text>();
					}
					else
					{ // bad syntax
						gtext().push_back(ev.ch);
						transit<PS_Text>();
					}
				}
			}

		protected:
			std::string m_name_chk;
		};

		class PS_NDBodyRemoteWaitClose : public ParseState
		{
		public:
			void react(PrsEvent const &ev) override
			{
				if (ev.ch == '/')
				{
					transit<PS_NDBodyRemoteClose>();
				}
				else
				{ // bad syntax
					gtext().push_back(ev.ch);
					transit<PS_Text>();
				}
			}
		};

		class PS_NDBodyText : public ParseState
		{
		public:
			void react(PrsEvent const &ev) override
			{
				if (ev.ch == '<')
				{
					gtext().push_back(ev.ch);
					transit<PS_NDBodyRemoteWaitClose>();
				}
				else
				{
					gndbody().m_text.push_back(ev.ch);
					gtext().push_back(ev.ch);
				}
			}
		};
	}


	/************************************************************************/
	/*                               PARSER                                 */
	/************************************************************************/
	class Parser
	{
		friend class _internal_ns::ParseState;
	public:
		static bool parse(Document& out, const std::string& content)
		{
			using namespace _internal_ns;

			bool retval = true;
			_reset_state();
			ParseState::start();
			for (const auto& ch : content)
			{
				ParseState::dispatch(PrsEvent(ch));
			}

			if (ParseState::is_in_state<_internal_ns::PS_Text>() &&
				gtext().length() > 0)
			{
				Node nd;
				nd.m_text = gtext();
				gdoc().push_back(nd);
			}

			out = gdoc();

			return retval;
		}

	protected:
#define _STATIC_MEMBER(_type, _name)	static _type& _name(){static _type _inst_; return _inst_;}
		_STATIC_MEMBER(std::string, gtext);
		_STATIC_MEMBER(Document, gdoc);
		_STATIC_MEMBER(Node, gndbody);
#undef  _STATIC_MEMBER

	protected:
		static void _reset_state()
		{
			gtext().clear();
			gndbody().reset();
		}
	};


	/************************************************************************/
	/*                      PARSER-STATE IMPLEMENTATION                     */
	/************************************************************************/
#define _IMPL_PS_STATIC_MEMBER(_type, _name) inline _type& _internal_ns::ParseState::_name() {return Parser::_name();}
	_IMPL_PS_STATIC_MEMBER(std::string, gtext);
	_IMPL_PS_STATIC_MEMBER(Document, gdoc);
	_IMPL_PS_STATIC_MEMBER(Node, gndbody);
#undef _IMPL_PS_STATIC_MEMBER
}



