// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>

#include "Common/Assertions.hpp"
#include "Common/MPI/PE.hpp"
#include "Common/Log.hpp"
#include "Math/LSS/Trilinos/TrilinosMatrix.hpp"
#include "Math/LSS/Trilinos/TrilinosVector.hpp"

/// @todo remove this header when done
#include "Common/MPI/debug.hpp"

////////////////////////////////////////////////////////////////////////////////////////////

/**
  @file TrilinosMatrix.cpp implementation of LSS::TrilinosMatrix
  @author Tamas Banyai

  It is based on Trilinos's FEVbrMatrix.
**/

////////////////////////////////////////////////////////////////////////////////////////////

using namespace CF;
using namespace CF::Math;
using namespace CF::Math::LSS;

////////////////////////////////////////////////////////////////////////////////////////////

TrilinosMatrix::TrilinosMatrix(const std::string& name) :
  LSS::Matrix(name),
  m_mat(0),
  m_is_created(false),
  m_neq(0),
  m_blockrow_size(0),
  m_blockcol_size(0),
  m_p2m(0),
  m_comm(Common::Comm::PE::instance().communicator())
{
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::create(CF::Common::Comm::CommPattern& cp, Uint neq, std::vector<Uint>& node_connectivity, std::vector<Uint>& starting_indices, LSS::Vector::Ptr solution, LSS::Vector::Ptr rhs)
{
  /// @todo structurally symmetricize the matrix
  /// @todo ensure main diagonal blocks always existent

  // get global ids vector
  int *gid=(int*)cp.gid()->pack();

  // prepare intermediate data
  int nmyglobalelements=0;
  int maxrowentries=0;
  std::vector<int> rowelements(0);
  std::vector<int> myglobalelements(0);

  for (int i=0; i<(const int)cp.isUpdatable().size(); i++)
    if (cp.isUpdatable()[i])
    {
      ++nmyglobalelements;
      myglobalelements.push_back((int)gid[i]);
      rowelements.push_back((int)(starting_indices[i+1]-starting_indices[i]));
      maxrowentries=maxrowentries<(starting_indices[i+1]-starting_indices[i])?(starting_indices[i+1]-starting_indices[i]):maxrowentries;
    }
  std::vector<double>dummy_entries(maxrowentries*neq*neq,0.);
  std::vector<int>global_columns(maxrowentries);

  // process local to matrix local numbering mapper
  int iupd=0;
  int ighost=nmyglobalelements;
  for (int i=0; i<(const int)cp.isUpdatable().size(); i++)
  {
    if (cp.isUpdatable()[i]) { m_p2m.push_back(iupd++); }
    else { m_p2m.push_back(ighost++); }
  }

PEDebugVector(m_p2m,m_p2m.size());

  // blockmaps (colmap is gid 1 to 1, rowmap is gid with ghosts filtered out)
  Epetra_BlockMap rowmap(-1,nmyglobalelements,&myglobalelements[0],neq,0,m_comm);
  Epetra_BlockMap colmap(-1,cp.isUpdatable().size(),gid,neq,0,m_comm);
  myglobalelements.clear();

  // create matrix
  m_mat=Teuchos::rcp(new Epetra_FEVbrMatrix(Copy,rowmap,colmap,&rowelements[0]));
/*must be a bug in Trilinos, Epetra_FEVbrMatrix constructor is in Copy mode but it hangs up anyway
  more funny, when it gets out of scope and gets dealloc'd, everything survives according to memcheck
  rowmap.~Epetra_BlockMap();
  colmap.~Epetra_BlockMap();
*/
  rowelements.clear();

  // prepare the entries
  for (int i=0; i<(const int)cp.isUpdatable().size(); i++)
    if (cp.isUpdatable()[i])
    {
      for(int j=(const int)starting_indices[i]; j<(const int)starting_indices[i+1]; j++) global_columns[j-starting_indices[i]]=gid[m_p2m[node_connectivity[j]]];
      TRILINOS_THROW(m_mat->BeginInsertGlobalValues(gid[i],(int)(starting_indices[i+1]-starting_indices[i]),&global_columns[0]));
      for(int j=(const int)starting_indices[i]; j<(const int)starting_indices[i+1]; j++)
        TRILINOS_THROW(m_mat->SubmitBlockEntry(&dummy_entries[0],0,neq,neq));
      TRILINOS_THROW(m_mat->EndSubmitEntries());
    }
  TRILINOS_THROW(m_mat->FillComplete());
  TRILINOS_THROW(m_mat->OptimizeStorage()); // in theory fillcomplete calls optimizestorage from Trilinos 8.x+
  delete[] gid;

  // set class properties
  m_is_created=true;
  m_neq=neq;
  m_blockrow_size=nmyglobalelements;
  m_blockcol_size=cp.gid()->size();
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::destroy()
{
  if (m_is_created) m_mat.reset();
  m_p2m.resize(0);
  m_p2m.reserve(0);
  m_neq=0;
  m_blockrow_size=0;
  m_blockcol_size=0;
  m_is_created=false;
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::set_value(const Uint icol, const Uint irow, const Real value)
{
  cf_assert(m_is_created);
  const int colblock=(int)m_p2m[icol/m_neq];
  const int colsub=(int)(icol%m_neq);
  const int rowblock=(int)m_p2m[irow/m_neq];
  const int rowsub=(int)(irow%m_neq);
  Epetra_SerialDenseMatrix **val;
  int* colindices;
  int blockrowsize;
  int dummyneq;
  TRILINOS_ASSERT(m_mat->ExtractMyBlockRowView(rowblock,dummyneq,blockrowsize,colindices,val));
  for (int i=0; i<(const int)blockrowsize; i++)
    if (*colindices++==colblock)
    {
      val[i][0](rowsub,colsub)=value;
      return;
    }
  throw Common::BadValue(FromHere(),"Trying to access an illegal entry.");
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::add_value(const Uint icol, const Uint irow, const Real value)
{
  cf_assert(m_is_created);
  const int colblock=(int)m_p2m[icol/m_neq];
  const int colsub=(int)(icol%m_neq);
  const int rowblock=(int)m_p2m[irow/m_neq];
  const int rowsub=(int)(irow%m_neq);
  Epetra_SerialDenseMatrix **val;
  int* colindices;
  int blockrowsize;
  int dummyneq;
  TRILINOS_ASSERT(m_mat->ExtractMyBlockRowView(rowblock,dummyneq,blockrowsize,colindices,val));
  for (int i=0; i<(const int)blockrowsize; i++)
    if (*colindices++==colblock)
    {
      val[i][0](rowsub,colsub)+=value;
      return;
    }
  throw Common::BadValue(FromHere(),"Trying to access an illegal entry.");
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::get_value(const Uint icol, const Uint irow, Real& value)
{
  cf_assert(m_is_created);
  const int colblock=(int)m_p2m[icol/m_neq];
  const int colsub=(int)(icol%m_neq);
  const int rowblock=(int)m_p2m[irow/m_neq];
  const int rowsub=(int)(irow%m_neq);
  Epetra_SerialDenseMatrix **val;
  int* colindices;
  int blockrowsize;
  int dummyneq;
  TRILINOS_ASSERT(m_mat->ExtractMyBlockRowView(rowblock,dummyneq,blockrowsize,colindices,val));
  for (int i=0; i<(const int)blockrowsize; i++)
    if (*colindices++==colblock)
    {
      value=val[i][0](rowsub,colsub);
      return;
    }
  throw Common::BadValue(FromHere(),"Trying to access an illegal entry.");
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::solve(LSS::Vector::Ptr solution, LSS::Vector::Ptr rhs)
{
  cf_assert(m_is_created);
  cf_assert(solution->is_created());
  cf_assert(rhs->is_created());

  /// @todo finish solve
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::set_values(const BlockAccumulator& values)
{
  cf_assert(m_is_created);
  const int numblocks=values.indices.size();
  int* idxs=(int*)&values.indices[0];
  for (int irow=0; irow<(const int)numblocks; irow++)
  {
    TRILINOS_ASSERT(m_mat->BeginReplaceMyValues(idxs[irow],numblocks,idxs));
    for (int icol=0; icol<numblocks; icol++)
      TRILINOS_ASSERT(m_mat->SubmitBlockEntry((double*)values.mat.data()+irow*m_neq+icol*m_neq*m_neq*numblocks,numblocks*m_neq,m_neq,m_neq));
    TRILINOS_ASSERT(m_mat->EndSubmitEntries());
  }
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::add_values(const BlockAccumulator& values)
{
/** @attention
 * issue1: its stupid to submitt each block separately, cant submit a single blockrow at once!!!
 * issue2: so it seems that on trilinos side there is another "blockaccumulator"
 * issue3: investigate performance if matrix is View mode, the copy due to issue2 could be circumvented (then epetra only stores pointers to values)
 * issue4: investigate prformance of extracting a blockrowview and fill manually, all blocks at once in the current blockrow
**/
  cf_assert(m_is_created);
  const int numblocks=values.indices.size();
  int* idxs=(int*)&values.indices[0];
  for (int irow=0; irow<(const int)numblocks; irow++)
  {
    TRILINOS_ASSERT(m_mat->BeginSumIntoMyValues(idxs[irow],numblocks,idxs));
    for (int icol=0; icol<numblocks; icol++)
      TRILINOS_ASSERT(m_mat->SubmitBlockEntry((double*)values.mat.data()+irow*m_neq+icol*m_neq*m_neq*numblocks,numblocks*m_neq,m_neq,m_neq));
    TRILINOS_ASSERT(m_mat->EndSubmitEntries());
  }
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::get_values(BlockAccumulator& values)
{
  /// @attention MODERATE EXPENSIVE, AVOID USAGE
  cf_assert(m_is_created);
  int* idxs=(int*)&values.indices[0];
  Epetra_SerialDenseMatrix **val;
  int* colindices;
  int blockrowsize;
  int dummy_neq;
  const int block_size=values.block_size();
  for (int i=0; i<(const int)block_size; i++)
  {
    TRILINOS_ASSERT(m_mat->ExtractMyBlockRowView(idxs[i],dummy_neq,blockrowsize,colindices,val));
    for (int j=0; j<(const int)blockrowsize; j++)
      for (int k=0; k<(const int)block_size; k++)
        if (idxs[k]==colindices[j])
          for (int l=0; l<(const int)m_neq; l++)
            for (int m=0; m<(const int)m_neq; m++)
              values.mat(i*m_neq+l,k*m_neq+m)=val[j][0](l,m);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::set_row(const Uint iblockrow, const Uint ieq, Real diagval, Real offdiagval)
{
  cf_assert(m_is_created);
  Epetra_SerialDenseMatrix **val;
  int* colindices;
  int blockrowsize;
  int diagonalblock=-1;
  int dummy_neq;
  TRILINOS_ASSERT(m_mat->ExtractMyBlockRowView((int)iblockrow,dummy_neq,blockrowsize,colindices,val));
  for (int i=0; i<blockrowsize; i++)
  {
    if (colindices[i]==iblockrow) diagonalblock=i;
    for (int j=0; j<m_neq; j++)
      val[i][0](ieq,j)=offdiagval;
  }
  val[diagonalblock][0](ieq,ieq)=diagval;
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::get_column_and_replace_to_zero(const Uint iblockcol, Uint ieq, std::vector<Real>& values)
{
  /// @note this could be made faster if structural symmetry is ensured during create, because then involved rows could be determined by indices in the ibloccol-th row
  /// @attention COMPUTATIONALLY VERY EXPENSIVE!
  cf_assert(m_is_created);
  values.resize(m_blockrow_size*m_neq);
  values.assign(m_blockrow_size*m_neq,0.);
  Epetra_SerialDenseMatrix **val;
  int* colindices;
  int blockrowsize;
  int dummy_neq;
PEDebugVector(m_p2m,m_p2m.size());
  for (int k=0; k<(const int)m_blockrow_size; k++)
  {
std::cout << k << " " << m_p2m[k] << "\n" << std::flush;
    TRILINOS_ASSERT(m_mat->ExtractMyBlockRowView(m_p2m[k],dummy_neq,blockrowsize,colindices,val));
    for (int i=0; i<blockrowsize; i++)
      if (colindices[i]==m_p2m[iblockcol])
      {
        for (int j=0; j<m_neq; j++)
        {
          values[k*m_neq+j]=val[i][0](j,ieq);
          val[i][0](j,ieq)=0.;
        }
        break;
      }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::tie_blockrow_pairs (const Uint iblockrow_to, const Uint iblockrow_from)
{
  cf_assert(m_is_created);
  Epetra_SerialDenseMatrix **val_to,**val_from;
  int* colindices_to, *colindices_from;
  int blockrowsize_to,blockrowsize_from;
  int diag=-1,pair=-1;
  int dummy_neq;
  TRILINOS_ASSERT(m_mat->ExtractMyBlockRowView((int)iblockrow_from,dummy_neq,blockrowsize_from,colindices_from,val_from));
  TRILINOS_ASSERT(m_mat->ExtractMyBlockRowView((int)iblockrow_to,  dummy_neq,blockrowsize_to,  colindices_to  ,val_to));
  if (blockrowsize_to!=blockrowsize_from) throw Common::BadValue(FromHere(),"Number of blocks do not match for the two block rows to be tied together.");
  for (int i=0; i<blockrowsize_to; i++)
  {
    cf_assert(colindices_to[i]==colindices_from[i]);
    if (colindices_from[i]==iblockrow_from) diag=i;
    if (colindices_to[i]  ==iblockrow_to)   pair=i;
    val_to[i][0]=val_from[i][0];
    val_from[i][0].Scale(0.);
  }
  for (int i=0; i<m_neq; i++)
  {
    val_from[diag][0](i,i)=1.;
    val_to[pair][0](i,i)=-1.;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::set_diagonal(const std::vector<Real>& diag)
{
  cf_assert(m_is_created);
  int *dummy_rowcoldim;
  double *entryvals;
  int numblocks,dummy_stride;
  const unsigned long stride=m_neq+1;
  const unsigned long blockadvance=m_neq*m_neq;
  TRILINOS_ASSERT(m_mat->BeginExtractBlockDiagonalView(numblocks,dummy_rowcoldim));
  cf_assert(diag.size()==numblocks*m_neq);
  double *diagvals=(double*)&diag[0];
  for (int i=0; i<(const int)numblocks; i++)
  {
    TRILINOS_ASSERT(m_mat->ExtractBlockDiagonalEntryView(entryvals,dummy_stride));
    for (double* ev=entryvals; ev<(const double*)(&entryvals[blockadvance]); ev+=stride)
      *ev=*diagvals++;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::add_diagonal(const std::vector<Real>& diag)
{
  cf_assert(m_is_created);
  int *dummy_rowcoldim;
  double *entryvals;
  int numblocks,dummy_stride;
  const unsigned long stride=m_neq+1;
  const unsigned long blockadvance=m_neq*m_neq;
  TRILINOS_ASSERT(m_mat->BeginExtractBlockDiagonalView(numblocks,dummy_rowcoldim));
  cf_assert(diag.size()==numblocks*m_neq);
  double *diagvals=(double*)&diag[0];
  for (int i=0; i<(const int)numblocks; i++)
  {
    TRILINOS_ASSERT(m_mat->ExtractBlockDiagonalEntryView(entryvals,dummy_stride));
    for (double* ev=entryvals; ev<(const double*)(&entryvals[blockadvance]); ev+=stride)
      *ev+=*diagvals++;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::get_diagonal(std::vector<Real>& diag)
{
  cf_assert(m_is_created);
  int *dummy_rowcoldim;
  double *entryvals;
  int numblocks,dummy_stride;
  const unsigned long stride=m_neq+1;
  const unsigned long blockadvance=m_neq*m_neq;
  TRILINOS_ASSERT(m_mat->BeginExtractBlockDiagonalView(numblocks,dummy_rowcoldim));
  diag.resize(numblocks*m_neq);
  double *diagvals=&diag[0];
  for (int i=0; i<(const int)numblocks; i++)
  {
    TRILINOS_ASSERT(m_mat->ExtractBlockDiagonalEntryView(entryvals,dummy_stride));
    for (double* ev=entryvals; ev<(const double*)(&entryvals[blockadvance]); ev+=stride)
      *diagvals++=*ev;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::reset(Real reset_to)
{
  cf_assert(m_is_created);
  m_mat->PutScalar(reset_to);
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::print(Common::LogStream& stream)
{
  if (m_is_created)
  {
    int sumentries=0;
    int rowentries,dummy_neq;
    int *idxs;
    Epetra_SerialDenseMatrix** vals;
    std::vector<int> m2p(m_blockcol_size,-1);
    for (int i=0; i<(const int)m_p2m.size(); i++) m2p[m_p2m[i]]=i;
    for (int i=0; i<(const int)m_mat->NumMyBlockRows(); i++)
    {
      TRILINOS_ASSERT(m_mat->ExtractMyBlockRowView(i,dummy_neq,rowentries,idxs,vals));
      for (int j=0; j<(const int)m_neq; j++)
        for (int k=0; k<(const int)rowentries; k++)
          for (int l=0; l<(const int)m_neq; l++)
            stream << m2p[idxs[k]]*m_neq+l << " " << -(int)(m2p[i]*m_neq+j) << " " << vals[k][0](j,l) << "\n";
      sumentries+=rowentries*m_neq*m_neq;
    }
    stream << "# name:                 " << name() << "\n";
    stream << "# type_name:            " << type_name() << "\n";
    stream << "# process:              " << m_comm.MyPID() << "\n";
    stream << "# number of equations:  " << m_neq << "\n";
    stream << "# number of rows:       " << m_blockrow_size*m_neq << "\n";
    stream << "# number of cols:       " << m_blockcol_size*m_neq << "\n";
    stream << "# number of block rows: " << m_blockrow_size << "\n";
    stream << "# number of block cols: " << m_blockcol_size << "\n";
    stream << "# number of entries:    " << sumentries << "\n";
  } else {
    stream << name() << " of type " << type_name() << "::is_created() is false, nothing is printed.";
  }
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::print(std::ostream& stream)
{
  if (m_is_created)
  {
    int sumentries=0;
    int rowentries,dummy_neq;
    int *idxs;
    Epetra_SerialDenseMatrix** vals;
    std::vector<int> m2p(m_blockcol_size,-1);
    for (int i=0; i<(const int)m_p2m.size(); i++) m2p[m_p2m[i]]=i;
    for (int i=0; i<(const int)m_mat->NumMyBlockRows(); i++)
    {
      TRILINOS_ASSERT(m_mat->ExtractMyBlockRowView(i,dummy_neq,rowentries,idxs,vals));
      for (int j=0; j<(const int)m_neq; j++)
        for (int k=0; k<(const int)rowentries; k++)
          for (int l=0; l<(const int)m_neq; l++)
            stream << m2p[idxs[k]]*m_neq+l << " " << -(int)(m2p[i]*m_neq+j) << " " << vals[k][0](j,l) << "\n" << std::flush;
      sumentries+=rowentries*m_neq*m_neq;
    }
    stream << "# name:                 " << name() << "\n";
    stream << "# type_name:            " << type_name() << "\n";
    stream << "# process:              " << m_comm.MyPID() << "\n";
    stream << "# number of equations:  " << m_neq << "\n";
    stream << "# number of rows:       " << m_blockrow_size*m_neq << "\n";
    stream << "# number of cols:       " << m_blockcol_size*m_neq << "\n";
    stream << "# number of block rows: " << m_blockrow_size << "\n";
    stream << "# number of block cols: " << m_blockcol_size << "\n";
    stream << "# number of entries:    " << sumentries << "\n";
  } else {
    stream << name() << " of type " << type_name() << "::is_created() is false, nothing is printed.";
  }
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::print(const std::string& filename, std::ios_base::openmode mode )
{
  std::ofstream stream(filename.c_str(),mode);
  stream << "ZONE T=\"" << type_name() << " " << name() << "\"\n" << std::flush;
  print(stream);
  stream.close();
}

////////////////////////////////////////////////////////////////////////////////////////////

void TrilinosMatrix::data(std::vector<Uint>& row_indices, std::vector<Uint>& col_indices, std::vector<Real>& values)
{
  cf_assert(m_is_created);
  row_indices.clear();
  col_indices.clear();
  values.clear();
  int rowentries,dummy_neq;
  int *idxs;
  Epetra_SerialDenseMatrix** vals;
  std::vector<int> m2p(m_blockcol_size,-1);
  for (int i=0; i<(const int)m_p2m.size(); i++) m2p[m_p2m[i]]=i;
  for (int i=0; i<(const int)m_mat->NumMyBlockRows(); i++)
  {
    TRILINOS_ASSERT(m_mat->ExtractMyBlockRowView(i,dummy_neq,rowentries,idxs,vals));
    for (int j=0; j<(const int)m_neq; j++)
      for (int k=0; k<(const int)rowentries; k++)
        for (int l=0; l<(const int)m_neq; l++)
        {
          row_indices.push_back((m2p[i]*m_neq+j));
          col_indices.push_back(m2p[idxs[k]]*m_neq+l);
          values.push_back(vals[k][0](j,l));
        }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////