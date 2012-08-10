//----------------------------  vectors_boundary_rhs.cc  ---------------------------
//    $Id$
//    Version: $Name$ 
//
//    Copyright (C) 2000, 2001, 2003, 2004, 2006, 2007 by the deal.II authors
//
//    This file is subject to QPL and may not be  distributed
//    without copyright and license information. Please refer
//    to the file deal.II/doc/license.html for the  text  and
//    further information on this license.
//
//----------------------------  vectors_boundary_rhs.cc  ---------------------------


// like deal.II/vectors_boundary_rhs, but for hp objects. here, each hp object has only a
// single component, so we expect exactly the same output as for the old test.
// vectors_boundary_rhs_hp tests for different finite elements


#include "../tests.h"
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/logstream.h>
#include <deal.II/base/function_lib.h>
#include <deal.II/lac/sparse_matrix.h>
#include <deal.II/lac/vector.h>
#include <deal.II/grid/tria.h>
#include <deal.II/grid/tria_iterator.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/hp/dof_handler.h>
#include <deal.II/dofs/dof_tools.h>
#include <deal.II/lac/constraint_matrix.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_system.h>
#include <deal.II/hp/fe_collection.h>
#include <deal.II/hp/q_collection.h>
#include <deal.II/fe/mapping_q.h>
#include <deal.II/hp/mapping_collection.h>
#include <deal.II/numerics/vector_tools.h>

#include <fstream>


template<int dim>
class MySquareFunction : public Function<dim>
{
  public:
    MySquareFunction () : Function<dim>(2) {}
    
    virtual double value (const Point<dim>   &p,
			  const unsigned int  component) const
      {	return (component+1)*p.square(); }
    
    virtual void   vector_value (const Point<dim>   &p,
				 Vector<double>     &values) const
      { values(0) = value(p,0);
	values(1) = value(p,1); }
};




template <int dim>
void
check ()
{
  Triangulation<dim> tr;  
  if (dim==2)
    GridGenerator::hyper_ball(tr, Point<dim>(), 1);
  else
    GridGenerator::hyper_cube(tr, -1,1);
  tr.refine_global (1);
  tr.begin_active()->set_refine_flag ();
  tr.execute_coarsening_and_refinement ();
  if (dim==1)
    tr.refine_global(2);

				   // create a system element composed
				   // of one Q1 and one Q2 element
  hp::FECollection<dim> element;
  element.push_back (FESystem<dim> (FE_Q<dim>(1), 1,
				    FE_Q<dim>(2), 1));
  hp::DoFHandler<dim> dof(tr);
  for (typename hp::DoFHandler<dim>::active_cell_iterator
	 cell = dof.begin_active(); cell!=dof.end(); ++cell)
    cell->set_active_fe_index (rand() % element.size());
  
  dof.distribute_dofs(element);

				   // use a more complicated mapping
				   // of the domain and a quadrature
				   // formula suited to the elements
				   // we have here
  hp::MappingCollection<dim> mapping;
  mapping.push_back (MappingQ<dim>(3));

  hp::QCollection<dim-1> quadrature;
  quadrature.push_back (QGauss<dim-1>(3));

  Vector<double> rhs (dof.n_dofs());
  VectorTools::create_boundary_right_hand_side (dof, quadrature,
				       MySquareFunction<dim>(),
				       rhs);
  for (unsigned int i=0; i<rhs.size(); ++i)
    deallog << rhs(i) << std::endl;
}



int main ()
{
  std::ofstream logfile ("vectors_boundary_rhs_01/output");
  logfile.precision (4);
  logfile.setf(std::ios::fixed);  
  deallog.attach(logfile);
  deallog.depth_console (0);

  deallog.push ("2d");
  check<2> ();
  deallog.pop ();
  deallog.push ("3d");
  check<3> ();
  deallog.pop ();
}
