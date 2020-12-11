/**
 * @file 	general_dynamics.cpp
 * @author	Luhui Han, Chi ZHang and Xiangyu Hu
 * @version	0.1
 */

#include "general_dynamics.h"

namespace SPH {
	//=================================================================================================//
	InitializeATimeStep
		::InitializeATimeStep(SPHBody* body, Gravity* gravity)
		: ParticleDynamicsSimple(body), GeneralDataDelegateSimple(body),
		pos_n_(particles_->pos_n_), dvel_dt_others_(particles_->dvel_dt_others_),
		gravity_(gravity)
	{
	}
	//=================================================================================================//
	void InitializeATimeStep::setupDynamics(Real dt)
	{
		particles_->number_of_ghost_particles_ = 0;
	}
	//=================================================================================================//
	void InitializeATimeStep::Update(size_t index_i, Real dt)
	{
		dvel_dt_others_[index_i] = gravity_->InducedAcceleration(pos_n_[index_i]);
	}
	//=================================================================================================//
	RandomizePartilePosition::RandomizePartilePosition(SPHBody* body)
		: ParticleDynamicsSimple(body), DataDelegateSimple<SPHBody, BaseParticles>(body),
		pos_n_(particles_->pos_n_)
	{
		particle_spacing_ = body->particle_spacing_;
	}
	//=================================================================================================//
	void RandomizePartilePosition::Update(size_t index_i, Real dt)
	{
		Vecd& pos_n_i = pos_n_[index_i];
		for (int k = 0; k < pos_n_i.size(); ++k)
		{
			pos_n_i[k] += dt * (((double)rand() / (RAND_MAX)) - 0.5) * 2.0 * particle_spacing_;
		}
	}
	//=================================================================================================//
	BoundingBodyDomain::BoundingBodyDomain(SPHBody* body)
		: ParticleDynamics<void>(body), DataDelegateSimple<SPHBody, BaseParticles>(body),
		pos_n_(particles_->pos_n_),
		cell_linked_lists_(mesh_cell_linked_list_->CellLinkedLists()),
		number_of_cells_(mesh_cell_linked_list_->NumberOfCells()),
		cell_spacing_(mesh_cell_linked_list_->CellSpacing()),
		mesh_lower_bound_(mesh_cell_linked_list_->MeshLowerBound())
	{
		body_->findBodyDomainBounds(body_lower_bound_, body_upper_bound_);
		SetCellBounds();
	}
	//=================================================================================================//
	void BoundingBodyDomain::SetCellBounds()
	{
		Vecd rltpos = body_lower_bound_ - mesh_lower_bound_;
		for (int i = 0; i < rltpos.size(); ++i) {
			body_lower_bound_cell_[i] = (size_t)floor(rltpos[i] / cell_spacing_);
			/**< floor in c++ Rounds x downward, returning the largest integral value
					that is not greater than x. */
		}

		rltpos = body_upper_bound_ - mesh_lower_bound_;
		for (int i = 0; i < rltpos.size(); ++i) {
			body_upper_bound_cell_[i] = (size_t)floor(rltpos[i] / cell_spacing_);
		}
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirection::setPeriodicTranslation()
	{
		periodic_translation_[axis_] = body_upper_bound_[axis_] - body_lower_bound_[axis_];
		if (periodic_translation_.norm() < body_->particle_spacing_) {
			std::cout << __FILE__ << ':' << __LINE__ << std::endl;
			std::cout << "\n Periodic bounding failure: bounds not defined!" << std::endl;
			exit(1);
		}
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirection::PeriodicBounding::checkLowerBound(size_t index_i, Real dt)
	{
		if (pos_n_[index_i][axis_] < body_lower_bound_[axis_])
			pos_n_[index_i][axis_] += periodic_translation_[axis_];
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirection::PeriodicBounding::checkUpperBound(size_t index_i, Real dt)
	{
		if (pos_n_[index_i][axis_] > body_upper_bound_[axis_])
			pos_n_[index_i][axis_] -= periodic_translation_[axis_];
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirection::exec(Real dt)
	{
		std::cout << "\n Error: each step should be called separately." << std::endl;
		std::cout << __FILE__ << ':' << __LINE__ << std::endl;
		exit(1);
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirection::parallel_exec(Real dt)
	{
		std::cout << "\n Error: each step should be called separately." << std::endl;
		std::cout << __FILE__ << ':' << __LINE__ << std::endl;
		exit(1);
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirectionUsingCellLinkedList::
		PeriodicCellLinkedList::checkUpperBound(ListData& list_data, Real dt)
	{
		Vecd particle_position = list_data.second;
		if (particle_position[axis_] < body_upper_bound_[axis_]
			&& particle_position[axis_] > (body_upper_bound_[axis_] - cell_spacing_))
		{
			Vecd translated_position = particle_position - periodic_translation_;
			/** insert ghost particle to cell linked list */
			mesh_cell_linked_list_->InsertACellLinkedListDataEntry(list_data.first, translated_position);
		}
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirectionUsingCellLinkedList::
		PeriodicCellLinkedList::checkLowerBound(ListData& list_data, Real dt)
	{
		Vecd particle_position = list_data.second;
		if (particle_position[axis_] > body_lower_bound_[axis_]
			&& particle_position[axis_] < (body_lower_bound_[axis_] + cell_spacing_))
		{
			Vecd translated_position = particle_position + periodic_translation_;
			/** insert ghost particle to cell linked list */
			mesh_cell_linked_list_->InsertACellLinkedListDataEntry(list_data.first, translated_position);
		}
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirectionUsingGhostParticles::
		CreatPeriodicGhostParticles::setupDynamics(Real dt)
	{
		for (size_t i = 0; i != ghost_particles_.size(); ++i) ghost_particles_[i].clear();
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirectionUsingGhostParticles::
		CreatPeriodicGhostParticles::checkLowerBound(size_t index_i, Real dt)
	{
		Vecd particle_position = pos_n_[index_i];
		if (particle_position[axis_] > body_lower_bound_[axis_]
			&& particle_position[axis_] < (body_lower_bound_[axis_] + cell_spacing_))
		{
			size_t expected_particle_index = particles_->insertAGhostParticle(index_i);
			ghost_particles_[0].push_back(expected_particle_index);
			Vecd translated_position = particle_position + periodic_translation_;
			/** insert ghost particle to cell linked list */
			mesh_cell_linked_list_->InsertACellLinkedListDataEntry(expected_particle_index, translated_position);
		}
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirectionUsingGhostParticles::
		CreatPeriodicGhostParticles::checkUpperBound(size_t index_i, Real dt)
	{
		Vecd particle_position = pos_n_[index_i];
		if (particle_position[axis_] < body_upper_bound_[axis_]
			&& particle_position[axis_] > (body_upper_bound_[axis_] - cell_spacing_))
		{
			size_t expected_particle_index = particles_->insertAGhostParticle(index_i);
			ghost_particles_[1].push_back(expected_particle_index);
			Vecd translated_position = particle_position - periodic_translation_;
			/** insert ghost particle to cell linked list */
			mesh_cell_linked_list_->InsertACellLinkedListDataEntry(expected_particle_index, translated_position);
		}
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirectionUsingGhostParticles::
		UpdatePeriodicGhostParticles::checkLowerBound(size_t index_i, Real dt)
	{
		particles_->updateFromAnotherParticle(index_i, sorted_id_[index_i]);
		pos_n_[index_i] += periodic_translation_;
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirectionUsingGhostParticles::
		UpdatePeriodicGhostParticles::checkUpperBound(size_t index_i, Real dt)
	{
		particles_->updateFromAnotherParticle(index_i, sorted_id_[index_i]);
		pos_n_[index_i] -= periodic_translation_;
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirectionUsingGhostParticles::
		UpdatePeriodicGhostParticles::exec(Real dt)
	{
		for (size_t i = 0; i != ghost_particles_[0].size(); ++i) 
		{
			checkLowerBound(ghost_particles_[0][i], dt);
		}
		for (size_t i = 0; i != ghost_particles_[1].size(); ++i)
		{
			checkUpperBound(ghost_particles_[1][i], dt);
		}
	}
	//=================================================================================================//
	void PeriodicConditionInAxisDirectionUsingGhostParticles::
		UpdatePeriodicGhostParticles::parallel_exec(Real dt)
	{
		parallel_for(blocked_range<size_t>(0, ghost_particles_[0].size()),
			[&](const blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i < r.end(); ++i) {
					checkLowerBound(ghost_particles_[0][i], dt);
				}
			}, ap);
	
		parallel_for(blocked_range<size_t>(0, ghost_particles_[1].size()),
			[&](const blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i < r.end(); ++i) {
					checkUpperBound(ghost_particles_[1][i], dt);
				}
			}, ap);
	}
	//=================================================================================================//
	MirrorBoundaryConditionInAxisDirection::MirrorBounding
		::MirrorBounding(CellVector& bound_cells, SPHBody* body, int axis_direction, bool positive)
		: BoundingInAxisDirection(body, axis_direction),
		bound_cells_(bound_cells), vel_n_(particles_->vel_n_)
	{
		checking_bound_ = positive ?
			std::bind(&MirrorBoundaryConditionInAxisDirection::MirrorBounding::checkUpperBound, this, _1, _2)
			: std::bind(&MirrorBoundaryConditionInAxisDirection::MirrorBounding::checkLowerBound, this, _1, _2);
	}
	//=================================================================================================//
	MirrorBoundaryConditionInAxisDirection
		::CreatingGhostParticles::CreatingGhostParticles(IndexVector& ghost_particles,
			CellVector& bound_cells, SPHBody* body, int axis_direction, bool positive)
		: MirrorBounding(bound_cells, body, axis_direction, positive), ghost_particles_(ghost_particles) {}
	//=================================================================================================//
	MirrorBoundaryConditionInAxisDirection::UpdatingGhostStates
		::UpdatingGhostStates(IndexVector& ghost_particles, CellVector& bound_cells,
			SPHBody* body, int axis_direction, bool positive)
		: MirrorBounding(bound_cells, body, axis_direction, positive), ghost_particles_(ghost_particles)
	{
		checking_bound_update_ = positive ?
			std::bind(&MirrorBoundaryConditionInAxisDirection::UpdatingGhostStates::checkUpperBound, this, _1, _2)
			: std::bind(&MirrorBoundaryConditionInAxisDirection::UpdatingGhostStates::checkLowerBound, this, _1, _2);
	}
	//=================================================================================================//
	void MirrorBoundaryConditionInAxisDirection
		::MirrorBounding::checkLowerBound(size_t index_i, Real dt)
	{
		if (pos_n_[index_i][axis_] < body_lower_bound_[axis_]) {
			mirrorInAxisDirection(index_i, body_lower_bound_, axis_);
		}
	}
	//=================================================================================================//
	void MirrorBoundaryConditionInAxisDirection::MirrorBounding
		::checkUpperBound(size_t index_i, Real dt)
	{
		if (pos_n_[index_i][axis_] > body_upper_bound_[axis_]) {
			mirrorInAxisDirection(index_i, body_upper_bound_, axis_);
		}
	}
	//=================================================================================================//
	void  MirrorBoundaryConditionInAxisDirection::MirrorBounding::
		mirrorInAxisDirection(size_t particle_index_i, Vecd body_bound, int axis_direction)
	{
		pos_n_[particle_index_i][axis_direction]
			= 2.0 * body_bound[axis_direction] - pos_n_[particle_index_i][axis_direction];
		vel_n_[particle_index_i][axis_direction] *= -1.0;
	}
	//=================================================================================================//
	void MirrorBoundaryConditionInAxisDirection
		::CreatingGhostParticles::checkLowerBound(size_t index_i, Real dt)
	{
		Vecd particle_position = pos_n_[index_i];
		if (particle_position[axis_] > body_lower_bound_[axis_]
			&& particle_position[axis_] < (body_lower_bound_[axis_] + cell_spacing_))
		{
			size_t expected_particle_index = particles_->insertAGhostParticle(index_i);
			ghost_particles_.push_back(expected_particle_index);
			/** mirror boundary condition */
			mirrorInAxisDirection(expected_particle_index, body_lower_bound_, axis_);
			Vecd translated_position = particles_->pos_n_[expected_particle_index];
			/** insert ghost particle to cell linked list */
			mesh_cell_linked_list_->InsertACellLinkedListDataEntry(expected_particle_index, translated_position);
		}
	}
	//=================================================================================================//
	void MirrorBoundaryConditionInAxisDirection
		::CreatingGhostParticles::checkUpperBound(size_t index_i, Real dt)
	{
		Vecd particle_position = pos_n_[index_i];
		if (particle_position[axis_] < body_upper_bound_[axis_]
			&& particle_position[axis_] > (body_upper_bound_[axis_] - cell_spacing_))
		{
			size_t expected_particle_index = particles_->insertAGhostParticle(index_i);
			ghost_particles_.push_back(expected_particle_index);
			/** mirror boundary condition */
			mirrorInAxisDirection(expected_particle_index, body_upper_bound_, axis_);
			Vecd translated_position = particles_->pos_n_[expected_particle_index];
			/** insert ghost particle to cell linked list */
			mesh_cell_linked_list_->InsertACellLinkedListDataEntry(expected_particle_index, translated_position);
		}
	}
	//=================================================================================================//
	void MirrorBoundaryConditionInAxisDirection::UpdatingGhostStates
		::checkLowerBound(size_t index_i, Real dt)
	{
		particles_->updateFromAnotherParticle(index_i, sorted_id_[index_i]);
		mirrorInAxisDirection(index_i, body_lower_bound_, axis_);
	}
	//=================================================================================================//
	void MirrorBoundaryConditionInAxisDirection::UpdatingGhostStates
		::checkUpperBound(size_t index_i, Real dt)
	{
		particles_->updateFromAnotherParticle(index_i, sorted_id_[index_i]);
		mirrorInAxisDirection(index_i, body_upper_bound_, axis_);
	}
	//=================================================================================================//
	void MirrorBoundaryConditionInAxisDirection::UpdatingGhostStates
		::exec(Real dt)
	{
		for (size_t i = 0; i != ghost_particles_.size(); ++i) {
			checking_bound_update_(ghost_particles_[i], dt);
		}
	}
	//=================================================================================================//
	void MirrorBoundaryConditionInAxisDirection::UpdatingGhostStates
		::parallel_exec(Real dt)
	{
		parallel_for(blocked_range<size_t>(0, ghost_particles_.size()),
			[&](const blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i < r.end(); ++i) {
					checking_bound_update_(ghost_particles_[i], dt);
				}
			}, ap);
	}
	//=================================================================================================//
	VelocityBoundCheck::
		VelocityBoundCheck(SPHBody* body, Real velocity_bound)
		: ParticleDynamicsReduce<bool, ReduceOR>(body),
		GeneralDataDelegateSimple(body),
		vel_n_(particles_->vel_n_), velocity_bound_(velocity_bound)
	{
		initial_reference_ = false;
	}
	//=================================================================================================//
	bool VelocityBoundCheck::ReduceFunction(size_t index_i, Real dt)
	{
		return vel_n_[index_i].norm() > velocity_bound_;
	}
	//=================================================================================================//
	UpperFrontInXDirection::
		UpperFrontInXDirection(SPHBody* body) :
		ParticleDynamicsReduce<Real, ReduceMax>(body),
		GeneralDataDelegateSimple(body),
		pos_n_(particles_->pos_n_)
	{
		initial_reference_ = 0.0;
	}
	//=================================================================================================//
	Real UpperFrontInXDirection::ReduceFunction(size_t index_i, Real dt)
	{
		return pos_n_[index_i][0];
	}
	//=================================================================================================//
	MaximumSpeed::
		MaximumSpeed(SPHBody* body) :
		ParticleDynamicsReduce<Real, ReduceMax>(body),
		GeneralDataDelegateSimple(body),
		vel_n_(particles_->vel_n_)
	{
		initial_reference_ = 0.0;
	}
	//=================================================================================================//
	Real MaximumSpeed::ReduceFunction(size_t index_i, Real dt)
	{
		return vel_n_[index_i].norm();
	}
	//=================================================================================================//
	BodyLowerBound::BodyLowerBound(SPHBody* body)
		: ParticleDynamicsReduce<Vecd, ReduceLowerBound>(body),
		GeneralDataDelegateSimple(body),
		pos_n_(particles_->pos_n_)
	{
		constexpr  double max_real_number = (std::numeric_limits<double>::max)();
		initial_reference_ = Vecd(max_real_number);
	}
	//=================================================================================================//
	Vecd BodyLowerBound::ReduceFunction(size_t index_i, Real dt)
	{
		return pos_n_[index_i];
	}
	//=================================================================================================//
	BodyUpperBound::
		BodyUpperBound(SPHBody* body) :
		ParticleDynamicsReduce<Vecd, ReduceUpperBound>(body),
		GeneralDataDelegateSimple(body),
		pos_n_(particles_->pos_n_) 
	{
		constexpr  double min_real_number = (std::numeric_limits<double>::min)();
		initial_reference_ = Vecd(min_real_number);
	}
	//=================================================================================================//
	Vecd BodyUpperBound::ReduceFunction(size_t index_i, Real dt)
	{
		return pos_n_[index_i];
	}
	//=================================================================================================//
}
//=================================================================================================//