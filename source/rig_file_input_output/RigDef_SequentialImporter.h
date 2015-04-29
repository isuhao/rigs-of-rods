/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2015 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/**
	@file   
	@author Petr Ohlidal
	@date   04/2015
*/

#pragma once

#include "RigDef_Prerequisites.h"
#include "RigDef_File.h"

#include <boost/shared_ptr.hpp>
#include <vector>

namespace RigDef
{

/* 
    @brief Legacy rig-file importer.

    RoR's physics work on this principle:
    1. There is a node array, pre-allocated to some 5000 items.
    2. As .truck file is parsed, defined nodes are reserved in this array and accessible through their 
       index. Alternatively, nodes can be defined "named"; in that case, a <name->node index> translation 
       is performed and node can be addressed by both.
    3. Nodes are also generated by some sections (cinecam, wheels...). These can also be adressed by index.

    Traditionally, the order of nodes in nodes-array was determined by order of definitions in .truck file.
    This approach had downsides:
    1. Nodes could be adressed before they were defined
       This could be detected, but for compatibility reasons, parser emitted a warning only.
    2. Non-existent nodes could be adressed.
       Due to pre-allocated nature of node-array, this passed unnoticed.
       It could be explicitly detected, but parsers weren't equipped to do so in every case.
    3. Very hard to determine index of some nodes.
       Creating rig was traditionally a manual process, and not every section supported named nodes (i.e. axles)
       Knowing index of node was necessary, but with named/generated nodes, it was hard to tell.
    3. Unfeasible for visual editor with mouse controls.
       This approach would, by nature, re-order definitions and make direct index-adressing impossible.

    To ease code maintenance, enable bullet-proof checking and prepare grounds for visual rig editor,
    a new approach was applied: Truckfile would be loaded to RigDef::File structure in it's entirety, 
    and then parsed in pre-determined order.
    Order (section name[number of nodes generated per line]):
        1. nodes[1]
        2. nodes2[1]
        3. cinecam[1]
        4. wheels[num_rays*2]
        5. wheels2[num_rays*4]
        6. meshwheels[num_rays*2]
        7. meshwheels2[num_rays*2]
        8. flexbodywheels[num_rays*4]
    
    Naturally, older truckfiles which rely on user-determined order of definitions needed to be converted
    with index-references updated to match new node-array positions. This class does exactly that.

*/
class SequentialImporter
{
public:
    struct NodeMapEntry
    {
        enum OriginDetail
        {
            DETAIL_WHEEL_TYRE_A,
            DETAIL_WHEEL_TYRE_B,
            DETAIL_WHEEL_RIM_A,
            DETAIL_WHEEL_RIM_B,

            DETAIL_UNDEFINED = 0xFFFFFFFF
        };

        NodeMapEntry(File::Keyword keyword, Node::Id id, unsigned node_sub_index, OriginDetail detail=NodeMapEntry::DETAIL_UNDEFINED):
            origin_keyword(keyword),
            origin_detail(detail),
            node_sub_index(node_sub_index),
            node_id(id)
        {}

        RigDef::File::Keyword origin_keyword;
        OriginDetail          origin_detail;
        Node::Id              node_id;
        unsigned              node_sub_index;
    };

    struct Message
	{
		enum Type
		{
            TYPE_INFO,
			TYPE_WARNING,
			TYPE_ERROR,
			TYPE_FATAL_ERROR,

			TYPE_INVALID = 0xFFFFFFFF
		};

		std::string      message;
		File::Keyword    keyword;
		Type             type;
		std::string      module_name;
	};

    void Init(bool enabled);

    void Disable()
    {
        m_enabled = false;
        m_all_nodes.clear();
    }

    inline bool IsEnabled() const { return m_enabled; }

    bool AddNumberedNode(unsigned int number);

    bool AddNamedNode(std::string const & name);

    void AddGeneratedNode(File::Keyword generated_from, NodeMapEntry::OriginDetail detail = NodeMapEntry::DETAIL_UNDEFINED);

    void GenerateNodesForWheel(File::Keyword generated_from, int num_rays, bool has_rigidity_node);

    /// Traverse whole rig definition and resolve all node references
    void Process(boost::shared_ptr<RigDef::File> def);

    int GetMessagesNumErrors()   const { return m_messages_num_errors;   }
    int GetMessagesNumWarnings() const { return m_messages_num_warnings; }
    int GetMessagesNumOther()    const { return m_messages_num_other;    }

    std::string ProcessMessagesToString();
    void LogNodeStatistics();
    void IterateAndLogAllNodes();

private:

    void ProcessModule(boost::shared_ptr<RigDef::File::Module> module);

    Node::Ref ResolveNode(Node::Ref const & noderef_in);
    Node::Ref ResolveNodeByIndex(unsigned int index);
    unsigned GetNodeArrayOffset(File::Keyword keyword);
    void ResolveNodeRanges(std::vector<Node::Range>& ranges);

    void AddMessage(Message::Type msg_type, std::string text);

    std::vector<NodeMapEntry>           m_all_nodes;
    std::map<std::string, NodeMapEntry> m_named_nodes;
    unsigned                            m_num_numbered_nodes;    
    unsigned                            m_num_named_nodes;
    unsigned                            m_num_cinecam_nodes;
    unsigned                            m_num_wheels_nodes;
    unsigned                            m_num_wheels2_nodes;
    unsigned                            m_num_meshwheels_nodes;
    unsigned                            m_num_meshwheels2_nodes;
    unsigned                            m_num_flexbodywheels_nodes;
    bool                                m_enabled;

    // Logging
    int                                 m_total_resolved;
    int                                 m_num_resolved_to_self;
    File::Keyword                       m_current_keyword;
    boost::shared_ptr<File::Module>     m_current_module;
    std::list<Message>                  m_messages;
    int                                 m_messages_num_errors;
    int                                 m_messages_num_warnings;
    int                                 m_messages_num_other;
};

} // namespace RigDef
