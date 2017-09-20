//-----------------------------------------------------------
//
//    Copyright (C) 2015 - 2016 by the deal.II authors
//
//    This file is part of the deal.II library.
//
//    The deal.II library is free software; you can use it, redistribute
//    it, and/or modify it under the terms of the GNU Lesser General
//    Public License as published by the Free Software Foundation; either
//    version 2.1 of the License, or (at your option) any later version.
//    The full text of the license can be found in the file LICENSE at
//    the top level of the deal.II distribution.
//
//-----------------------------------------------------------

#include <deal.II/base/parameter_acceptor.h>
#include <deal.II/base/utilities.h>
#include <deal.II/base/revision.h>
#include <deal.II/base/path_search.h>
#include <boost/core/demangle.hpp>
#include <fstream>

DEAL_II_NAMESPACE_OPEN


// Static empty class list
std::vector<SmartPointer<ParameterAcceptor> > ParameterAcceptor::class_list;
// Static parameter handler
ParameterHandler ParameterAcceptor::prm;

ParameterAcceptor::ParameterAcceptor(const std::string name) :
  acceptor_id(class_list.size()),
  section_name(name)
{
  SmartPointer<ParameterAcceptor> pt(this, boost::core::demangle(typeid(*this).name()).c_str());
  class_list.push_back(pt);
}


ParameterAcceptor::~ParameterAcceptor()
{
  class_list[acceptor_id] = 0;
}

std::string ParameterAcceptor::get_section_name() const
{
  return (section_name != "" ? section_name : boost::core::demangle(typeid(*this).name()));
}


void
ParameterAcceptor::initialize(const std::string filename,
                              const std::string output_filename,
                              const ParameterHandler::OutputStyle output_style_for_prm_format)
{
  declare_all_parameters();
  // check the extension of input file
  if (filename.substr(filename.find_last_of(".") + 1) == "prm")
    {
      try
        {
          prm.parse_input(filename);
        }
      catch (dealii::PathSearch::ExcFileNotFound)
        {
          std::ofstream out(filename);
          Assert(out, ExcIO());
          prm.print_parameters(out, ParameterHandler::Text);
          out.close();
          AssertThrow(false, ExcMessage("You specified "+filename+" as input "+
                                        "parameter file, but it does not exist. " +
                                        "We created one for you."));
        }
    }
  else if (filename.substr(filename.find_last_of(".") + 1) == "xml")
    {
      std::ifstream is(filename);
      if (!is)
        {
          std::ofstream out(filename);
          Assert(out, ExcIO());
          prm.print_parameters(out, ParameterHandler::XML);
          out.close();
          is.clear();
          AssertThrow(false, ExcMessage("You specified "+filename+" as input "+
                                        "parameter file, but it does not exist. " +
                                        "We created one for you."));
        }
      prm.parse_input_from_xml(is);
    }
  else
    AssertThrow(false, ExcMessage("Invalid extension of parameter file. Please use .prm or .xml"));

  parse_all_parameters();
  if (output_filename != "")
    {
      std::ofstream outfile(output_filename.c_str());
      Assert(outfile, ExcIO());
      std::string extension = output_filename.substr(output_filename.find_last_of(".") + 1);

      if ( extension == "prm")
        {
          outfile << "# Parameter file generated with " << std::endl
                  << "# DEAL_II_GIT_BRANCH=   " << DEAL_II_GIT_BRANCH  << std::endl
                  << "# DEAL_II_GIT_SHORTREV= " << DEAL_II_GIT_SHORTREV << std::endl;
          Assert(output_style_for_prm_format == ParameterHandler::Text ||
                 output_style_for_prm_format == ParameterHandler::ShortText,
                 ExcMessage("Only Text or ShortText can be specified in output_style_for_prm_format."))
          prm.print_parameters(outfile, output_style_for_prm_format);
        }
      else if (extension == "xml")
        prm.print_parameters(outfile, ParameterHandler::XML);
      else if (extension == "latex" || extension == "tex")
        prm.print_parameters(outfile, ParameterHandler::LaTeX);
      else
        AssertThrow(false,ExcNotImplemented());
    }
}

void
ParameterAcceptor::clear()
{
  class_list.clear();
  prm.clear();
}



void ParameterAcceptor::declare_parameters(ParameterHandler &prm)
{}



void ParameterAcceptor::parse_parameters(ParameterHandler &prm)
{}



void
ParameterAcceptor::log_info()
{
  deallog.push("ParameterAcceptor");
  for (unsigned int i=0; i<class_list.size(); ++i)
    {
      deallog << "Class " << i << ":";
      if (class_list[i])
        deallog << class_list[i]->get_section_name() << std::endl;
      else
        deallog << " NULL" << std::endl;
    }
  deallog.pop();
}

void ParameterAcceptor::parse_all_parameters(ParameterHandler &prm)
{
  for (unsigned int i=0; i< class_list.size(); ++i)
    if (class_list[i] != NULL)
      {
        class_list[i]->enter_my_subsection(prm);
        class_list[i]->parse_parameters(prm);
        class_list[i]->parse_parameters_call_back();
        class_list[i]->leave_my_subsection(prm);
      }
}

void ParameterAcceptor::declare_all_parameters(ParameterHandler &prm)
{
  for (unsigned int i=0; i< class_list.size(); ++i)
    if (class_list[i] != NULL)
      {
        class_list[i]->enter_my_subsection(prm);
        class_list[i]->declare_parameters(prm);
        class_list[i]->declare_parameters_call_back();
        class_list[i]->leave_my_subsection(prm);
      }
}


std::vector<std::string>
ParameterAcceptor::get_section_path() const
{
  Assert(acceptor_id < class_list.size(), ExcInternalError());
  std::vector<std::string> sections =
    Utilities::split_string_list(class_list[acceptor_id]->get_section_name(), sep);
  bool is_absolute = false;
  if (sections.size() > 1)
    {
      // Handle the cases of a leading "/"
      if (sections[0] == "")
        {
          is_absolute = true;
          sections.erase(sections.begin());
        }
    }
  if (is_absolute == false)
    {
      // In all other cases, we scan for earlier classes, and prepend the
      // first absolute path (in reverse order) we find to ours
      for (int i=acceptor_id-1; i>=0; --i)
        if (class_list[i] != NULL)
          if (class_list[i]->get_section_name().front() == sep)
            {
              bool has_trailing = class_list[i]->get_section_name().back() == sep;
              // Absolute path found
              auto secs = Utilities::split_string_list(class_list[i]->get_section_name(), sep);
              Assert(secs[0] == "", ExcInternalError());
              // Insert all sections except first and last
              sections.insert(sections.begin(), secs.begin()+1, secs.end()-(has_trailing ? 0 : 1));
              // exit from for cycle
              break;
            }
    }
  return sections;
}

void ParameterAcceptor::enter_my_subsection(ParameterHandler &prm=ParameterAcceptor::prm)
{
  std::vector<std::string> sections = get_section_path();
  for (auto sec : sections)
    {
      prm.enter_subsection(sec);
    }
}

void ParameterAcceptor::leave_my_subsection(ParameterHandler &prm=ParameterAcceptor::prm)
{
  std::vector<std::string> sections = get_section_path();
  for (auto sec : sections)
    {
      prm.leave_subsection();
    }
}



DEAL_II_NAMESPACE_CLOSE

