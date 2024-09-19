// This example code reads a profiling file, processes it,
// and optionally dumps the reader profiling data itself to another file.
// It also prints a tree structure of the profiling data to the console.

#include <easy/profiler.h>
#include <easy/reader.h>
#include <fstream>
#include <list>
#include <iostream>
#include <map>
#include <stack>
#include <vector>
#include <iterator>
#include <algorithm> 
#include <ctime>
#include <chrono>
#include <string>
#include <sstream>

class TreePrinter
{
    const profiler::blocks_t& blocks;

    struct Info
    {
        std::string name;
        std::string info;
    };
    std::vector<Info> m_rows;

    void addNewRow(int level, const std::string& name, const std::string& info)
    {
        m_rows.push_back({std::string(level, '\t') + name, info});
    }

	void printTree(const profiler::BlocksTree& tree, int level = 0,
		profiler::timestamp_t parent_dur = 0, profiler::timestamp_t root_dur = 0)
	{
		if (tree.node)
		{
		    auto duration = tree.node->duration();
		    float duration_ms = duration / 1e6f;
		    float percent = parent_dur ? float(duration) / float(parent_dur) * 100.0f : 100.0f;
		    float rpercent = root_dur ? float(duration) / float(root_dur) * 100.0f : 100.0f;
		    std::stringstream info;
		    info << percent << "%| " << rpercent << "%| " << duration_ms << " ms";
		    addNewRow(level, tree.node->name(), info.str());
		    if (root_dur == 0)
		        root_dur = tree.node->duration();
		}
		else
		{
		    root_dur = 0;
		}

		for (auto i : tree.children)
		    printTree(blocks[i], level + 1, tree.node ? tree.node->duration() : 0, root_dur);
	}

public :

    TreePrinter(const profiler::blocks_t& blocks_) : blocks(blocks_) { }

    void print(const profiler::BlocksTreeRoot& tree)
    {
		for (auto i : tree.children)
        	printTree(blocks[i]);

        for (auto& row : m_rows)
            std::cout << row.name << " " << row.info << std::endl;
    }
};

int main(int argc, char* argv[])
{
    profiler::thread_blocks_tree_t threaded_trees;

    ::std::string filename;
    if (argc > 1 && argv[1])
    {
        filename = argv[1];
    }
    else
    {
        std::cout << "Usage: " << argv[0] << " <input_prof_file> [<output_prof_file>]";
        return 0;
        std::getline(std::cin, filename);
    }

    ::std::string dump_filename;
    if (argc > 2 && argv[2])
    {
        dump_filename = argv[2];
    }

    if (dump_filename.size())
    {
        EASY_PROFILER_ENABLE;
        std::cout << "Will dump reader prof file to " << dump_filename << std::endl;
    }

    auto start = std::chrono::system_clock::now();

    profiler::SerializedData serialized_blocks, serialized_descriptors;
    profiler::descriptors_list_t descriptors;
    profiler::blocks_t blocks;
    profiler::bookmarks_t bookmarks;
    profiler::BeginEndTime beginEndTime;
    std::stringstream errorMessage;
    uint32_t descriptorsNumberInFile = 0;
    uint32_t version = 0;
    profiler::processid_t pid = 0;

    auto blocks_counter = fillTreesFromFile(filename.c_str(), beginEndTime, serialized_blocks, serialized_descriptors,
                                            descriptors, blocks, threaded_trees, bookmarks, descriptorsNumberInFile,
                                            version, pid, true, errorMessage);
    if (blocks_counter == 0)
    {
        std::cout << "Can not read blocks from file " << filename.c_str() << "\nReason: " << errorMessage.str();
        return 1;
    }

    auto end = std::chrono::system_clock::now();

    std::cout << "Blocks count: " << blocks_counter << std::endl;
    std::cout << "dT =  " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " usec" << std::endl;

    for (const auto & i : threaded_trees)
    {
        std::cout << std::string(20, '=') << " thread " << i.first << " " << std::string(20, '=') << std::endl;
        TreePrinter printer(blocks);
        printer.print(i.second);
    }

    if (!dump_filename.empty())
    {
        auto bcount = profiler::dumpBlocksToFile(dump_filename.c_str());
        std::cout << "Blocks count for reader: " << bcount << std::endl;
    }

    return 0;
}
