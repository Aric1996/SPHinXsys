/**
 * @file 	body_relation.hpp
 * @brief 	Here, Functions belong to BaseBody, RealBody and FictitiousBody are given.
 * @author	hi ZHang and Xiangyu Hu
 * @version	0.1
 * 			0.2.0
 * 			Cell splitting algorithm are added.
 * 			Chi Zhang
 */
#include "body_relation.h"
#include "base_particles.h"
#include "base_kernel.h"
#include "mesh_cell_linked_list.h"

namespace SPH
{
	//=================================================================================================//
	template<typename GetParticleIndex>
	void SPHBodyContactRelation::
		updateConfigurationForParticles(size_t number_of_particles, GetParticleIndex& get_particle_index)
	{
		BaseParticles* base_particles = sph_body_->base_particles_;
		for (size_t relation_body_num = 0; relation_body_num < contact_sph_bodies_.size(); ++relation_body_num)
		{
			BaseMeshCellLinkedList& target_mesh_cell_linked_list
				= *(target_mesh_cell_linked_lists_[relation_body_num]);
			Vecu target_number_of_cells = target_mesh_cell_linked_list.NumberOfCells();
			int search_range = 
				mesh_cell_linked_list_->computeSearchRange(sph_body_->refinement_level_,
					contact_sph_bodies_[relation_body_num]->refinement_level_);
			Kernel& current_kernel = mesh_cell_linked_list_->ChoosingKernel(sph_body_->kernel_,
				contact_sph_bodies_[relation_body_num]->kernel_);
			Real cutoff_radius_sqr = powern(current_kernel.GetCutOffRadius(), 2);

			parallel_for(blocked_range<size_t>(0, number_of_particles),
				[&](const blocked_range<size_t>& r) {
					for (size_t num = r.begin(); num != r.end(); ++num) {
						
						size_t index_i = get_particle_index(num);
						Vecd particle_position = base_particles->pos_n_[index_i];
						Vecu target_cell_index = target_mesh_cell_linked_list
							.GridIndexFromPosition(particle_position);
						int i = (int)target_cell_index[0];
						int j = (int)target_cell_index[1];
						matrix_cell target_cell_linked_lists
							= target_mesh_cell_linked_list.CellLinkedLists();

						Neighborhood& neighborhood = contact_configuration_[relation_body_num][index_i];
						size_t current_count_of_neighbors = 0;
						for (int l = SMAX(i - search_range, 0); l <= SMIN(i + search_range, int(target_number_of_cells[0]) - 1); ++l)
							for (int m = SMAX(j - search_range, 0); m <= SMIN(j + search_range, int(target_number_of_cells[1]) - 1); ++m)
							{
								CellListDataVector& target_particles = target_cell_linked_lists[l][m].cell_list_data_;
								for (size_t n = 0; n < target_particles.size(); n++)
								{
										//displacement pointing from neighboring particle to origin particle
									Vecd displacement = particle_position - target_particles[n].second;
									if (displacement.normSqr() <= cutoff_radius_sqr)
									{
										current_count_of_neighbors >= neighborhood.memory_size_ ?
											createNeighborRelation(neighborhood,
												current_kernel, displacement, index_i, target_particles[n].first)
											: initializeNeighborRelation(neighborhood, current_count_of_neighbors,
												current_kernel, displacement, index_i, target_particles[n].first);
										current_count_of_neighbors++;
									}
								}
							}
						neighborhood.current_size_ = current_count_of_neighbors;
					}
				}, ap);
		}
	}
	//=================================================================================================//
}
