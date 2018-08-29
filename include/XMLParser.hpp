#pragma once
#include "PCH.hpp"
#include "File.hpp"
#include "rapidxml.hpp"

using namespace rapidxml;

class XML
{
public:
	XML() {}
	~XML() {}

	std::string xmlString;
	xml_document<> doc;

	union Binary
	{
		glm::fvec4 f4;
		glm::fvec3 f3;
		glm::fvec2 f2;
		float f;

		glm::ivec4 i4;
		glm::ivec3 i3;
		glm::ivec2 i2;
		s32 i;

		glm::uvec4 u4;
		glm::uvec3 u3;
		glm::uvec2 u2;
		u32 u;
	};

	struct Attribute
	{
		std::string value;
		Binary binary;
	};

	struct Node
	{
		std::list<Attribute> attribs;
		std::string value;
		Binary binary;

		std::list<Node*> children;
	};

	struct Parsed
	{
		std::list<Node*> nodes;
	};

	Parsed parsed;

	void parseFileFast(std::string path)
	{
		File file;
		file.open(path, File::in);

		xmlString.reserve(file.getSize());
		xmlString.assign((std::istreambuf_iterator<char>(file.fstream())), std::istreambuf_iterator<char>());

		doc.parse<0>((char*)xmlString.c_str());

		file.close();
	}

	void parseFile(std::string path)
	{
		/// TODO: fill the parsed struct
	}

	xml_node<>* firstNode(xml_node<>* parent, std::string name)
	{
		return parent->first_node(name.c_str());
	}

	xml_node<>* nextNode(xml_node<>* cur, std::string name)
	{
		return cur->next_sibling(name.c_str());
	}

	xml_attribute<>* att(xml_node<>* node, std::string name)
	{
		return node->first_attribute(name.c_str());
	}

	void getString(xml_base<>* base, std::string& ret)
	{
		if (!base->value())
			return;
		ret.assign(base->value(), base->value_size());
	}

	std::string getString(xml_base<>* base)
	{
		std::string ret;
		if (!base)
			return ret;
		if (!base->value())
			return ret;
		ret.assign(base->value(), base->value_size());
		return ret;
	}
};