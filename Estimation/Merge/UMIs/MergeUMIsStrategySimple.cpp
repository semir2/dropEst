#include "MergeUMIsStrategySimple.h"

#include <Tools/Logs.h>
#include <Tools/UtilFunctions.h>

#include <limits>

namespace Estimation
{
namespace Merge
{
namespace UMIs
{

MergeUMIsStrategySimple::MergeUMIsStrategySimple(unsigned int max_merge_distance)
		: _max_merge_distance(max_merge_distance)
{
	srand(42);
}

void MergeUMIsStrategySimple::merge(CellsDataContainer &container) const
{
	Tools::trace_time("Merge UMIs with N's");
	size_t total_cell_merged = 0, total_umi_merged = 0, total_cells_processed = 0;
	for (size_t cell_id = 0; cell_id < container.total_cells_number(); ++cell_id)
	{
		auto const &cell = container.cell(cell_id);
		if (!cell.is_real())
			continue;

		total_cells_processed++;
		for (auto const &gene : cell.genes())
		{
			s_hash_t bad_umis;
			for (auto const &umi : gene.second.umis())
			{
				auto umi_seq = container.umi_indexer().get_value(umi.first);
				if (this->is_umi_real(umi_seq))
					continue;

				bad_umis.insert(umi_seq);
			}

			if (bad_umis.empty())
				continue;

			auto merge_targets = this->find_targets(container.umi_indexer(), gene.second.umis(), bad_umis);

			total_cell_merged++;
			total_umi_merged += merge_targets.size();

			container.merge_umis(cell_id, gene.first, merge_targets);
		}
	}

	L_TRACE << total_cells_processed << " cells processed. Merged " << total_umi_merged << " UMIs from "
	        << total_cell_merged << " cells.";
	Tools::trace_time("UMI merge finished");
}

bool MergeUMIsStrategySimple::is_umi_real(const std::string &umi) const
{
	return umi.find('N') == std::string::npos;
}

CellsDataContainer::s_s_hash_t MergeUMIsStrategySimple::find_targets(const StringIndexer &umi_indexer,
                                                                     const Gene::umis_t &all_umis,
                                                                     const s_hash_t &bad_umis) const
{
	CellsDataContainer::s_s_hash_t merge_targets;
	for (auto const &bad_umi : bad_umis)
	{
		int min_ed = std::numeric_limits<unsigned>::max();
		std::string best_target = "";
		long best_target_size = 0;
		for (auto const &target_umi : all_umis)
		{
			auto target_umi_seq = umi_indexer.get_value(target_umi.first);
			if (bad_umis.find(target_umi_seq) != bad_umis.end())
				continue;

			unsigned ed = Tools::hamming_distance(target_umi_seq, bad_umi);
			if (ed < min_ed || (ed == min_ed && target_umi.second.read_count() > best_target_size))
			{
				min_ed = ed;
				best_target = target_umi_seq;
				best_target_size = target_umi.second.read_count();
			}
		}

		if (best_target == "" || min_ed > this->_max_merge_distance)
		{
			merge_targets[bad_umi] = MergeUMIsStrategyAbstract::fix_n_umi_with_random(bad_umi);
		}
		else
		{
			merge_targets[bad_umi] = best_target;
		}
	}

	return merge_targets;
}

CellsDataContainer::s_s_hash_t MergeUMIsStrategySimple::fill_wrong_umis(s_vec_t &wrong_umis) const
{
	CellsDataContainer::s_s_hash_t merge_targets;
	for (auto &umi : wrong_umis)
	{
		merge_targets[umi] = MergeUMIsStrategyAbstract::fix_n_umi_with_random(umi);
	}
	return merge_targets;
}
}
}
}