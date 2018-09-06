#include "../inc/tinyLMP.hpp"
#include <tinyfsm.hpp>
#include <iostream>

FSM_INITIAL_STATE(tinylmp::_internal_ns::ParseState, tinylmp::_internal_ns::PS_Text)

void dump_doc(const tinylmp::Document& doc)
{
	using namespace std;
	using namespace tinylmp;

	cout << "=---------------- dump doc ----------------=" << endl;
	cout << "node count = " << doc.node_count() << endl;
	for(size_t i = 0; i < doc.node_count(); ++i)
	{
		Node nd = doc.node_at(i);
		cout << "[node " << i << " | name = "
			<< nd.m_name << "| text = "
			<< nd.m_text << "]" << endl;
	}
}

int main()
{
	using namespace tinylmp;

	std::string test_ct = "aaa<bbb/>ccc<ddd>eee</ddd>";
	Document doc;

	std::cout << "src content : " << test_ct << std::endl;
	bool res = Parser::parse(doc, test_ct);
	if(res)
	{
		dump_doc(doc);
	}
	else
	{
		std::cout << "parse failed." << std::endl;	
	}
	
	return 0;
}