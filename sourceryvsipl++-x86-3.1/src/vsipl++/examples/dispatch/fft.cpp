/* Copyright (c) 2009 by CodeSourcery.  All rights reserved. */

/// Description: VSIPL++ Library: Custom evaluator.
///
/// This example illustrates the process of creating a custom FFT
/// evaluator.  FFT evaluators are more complex than simple function
/// evaluators, because they must create a VSIPL++ Fft functor object
/// rather than simply processing data.

#include <vsip/initfin.hpp>
#include <vsip/vector.hpp>
#include <vsip_csl/dda.hpp>
#include <vsip_csl/fft.hpp>
#include <iostream>

using namespace vsip;
namespace fft = vsip_csl::fft;


// ====================================================================
// Define a custom Fft_backend class for evaluating complex forward
// FFTs.  Sourcery VSIPL++ Fft objects encapsulate Fft_backend objects,
// which contain the methods to compute the FFT along with any 
// persistent data structures.  These are derived from specializations
// of the vsip_csl::fft::Fft_backend class.  The template parameters of
// the Fft_backend class describe the dimensionality, the input and
// output data types, and the direction of the FFT computation.
//
// For purposes of this example, we will define an FFT backend that
// is limited to computing FFTs of length 1024, with the complex data
// supplied in interleaved-complex format.
namespace example
{
class Fft_1024
  : public fft::Fft_backend<1, complex<float>, complex<float>, fft_fwd>
{
  // Fft_backend objects typically contain persistent internal data
  // such as twiddle factors, working arrays, or tuning information.
  // In this case, we will only store a scale factor.
  float scale_;

public:

  // The constructor for the Fft_backend object sets up the internal
  // data elements, based on the size of the FFT to be computed and a
  // scalar scaling factor for the output.
  Fft_1024(Domain<1> const &dom ATTRIBUTE_UNUSED, float scale)
    : scale_(scale)
  {
    // Print a diagnostic console message to indicate that this
    // constructor has been called.
    std::cout << "examples::Fft_1024 constructor called" << std::endl;
  }

  // Fft_backend objects must define two methods for computing the
  // FFT; one for in-place data and one for out-of-place data.  For
  // complex data, these methods may be defined either for interleaved
  // data or split data, or for both; this must match the layout 
  // defined in the query_layout methods (see below).  In this example,
  // we will only use interleaved data.
  //
  // For conciseness in this example, we will simply use skeleton
  // functions that do not really compute an FFT.
  //
  // The first of the two methods computes an in-place FFT.  This will
  // be used for single-argument by_reference Fft calls.
  void in_place(complex<float> *data, stride_type stride,
		length_type length)
  {
    // The FFT computation would go here.
    for (index_type i = 0; i < length; i++)
    {
      data[i * stride] = data[i * stride] * scale_;
    }

    // Print a diagnostic console message to indicate that this FFT
    // computation function has been called.
    std::cout << "examples::Fft_1024::in_place called" << std::endl;
  }

  // The second method computes an out-of-place FFT.  This will be used
  // for two-argument by_reference Fft calls and all by_value Fft
  // calls.
  void out_of_place(complex<float> *data_in, stride_type stride_in,
		    complex<float> *data_out, stride_type stride_out,
		    length_type length)
  {
    // Again, the FFT computation would go here.
    for (index_type i = 0; i < length; i++)
    {
      data_out[i * stride_out] = data_in[i * stride_in] * scale_;
    }

    // Print a diagnostic console message to indicate that this FFT
    // computation function has been called.
    std::cout << "examples::Fft_1024::out_of_place called" << std::endl;
  }

  // Fft_backend objects also contain query_layout methods, which 
  // describe the requirements for the layout of incoming data.  These 
  // methods receive dda::Rt_layout objects that describe the existing
  // layout of the data, and then modify these objects to describe how
  // the data should be rearranged if necessary.
  //
  // For in-place data, this backend requires unit-stride data in
  // complex interleaved format.
  void query_layout(vsip_csl::dda::Rt_layout<1> &rtl_inout)
  {
    rtl_inout.packing = dense;
    rtl_inout.storage_format = interleaved_complex;
  }

  // Similarly, for out-of-place data, this backend requires unit
  // strides and complex interleaved format in both the input and
  // output data.
  void query_layout(vsip_csl::dda::Rt_layout<1> &rtl_in, vsip_csl::dda::Rt_layout<1> &rtl_out)
  {
    rtl_in.packing = rtl_out.packing = dense;
    rtl_in.storage_format = rtl_out.storage_format = interleaved_complex;
  }
};

} // namespace example


// ====================================================================
// Having defined an Fft_backend class, we also must provide an
// Evaluator specialization that maps complex FFTs for 1024-element
// vectors to this backend.  This is quite similar to the Evaluator
// specializations for matrix products, except that the operation tag
// has template arguments, and the evaluation signature (the third
// template argument) returns a std::auto_ptr to a Fft_backend object.
//
// This evaluator applies to by_reference calls of the Fft object; to
// cover in_place calls, we could either define an second Evaluator
// specialization for that case or make the by_reference template
// argument into a template parameter alongside N.
namespace vsip_csl
{
namespace dispatcher
{
template <unsigned N>
struct Evaluator<op::fft<1, complex<float>, complex<float>, fft_fwd,
		 by_reference, N>,
  be::user,
  std::auto_ptr<fft::Fft_backend<1, complex<float>, complex<float>, fft_fwd> >
  (Domain<1> const &, float)>
{
  // At compile time for the Fft constructor, sizes and incoming data
  // layouts are unknown, so this backend is potentially valid for all
  // cases.
  static bool const ct_valid = true;

  // At runtime, we check for data of length 1024.  Note that this is
  // computed at the point where the Fft object is being constructed,
  // so we have no information about the incoming data layouts and
  // cannot use them to determine validity.
  static bool rt_valid(Domain<1> const &dom, float)
  { 
    return dom.size() == 1024;
  }

  // The exec method simply calls our backend function to create a new
  // Fft_backend object.  Note that any profiling added here will only
  // profile the construction of the Fft object, not its execution to
  // compute FFTs.
  static std::auto_ptr<fft::Fft_backend<1, complex<float>, complex<float>, fft_fwd> >
  exec(Domain<1> const &dom, float scale)
  {
    return std::auto_ptr<fft::Fft_backend<1, complex<float>, complex<float>, fft_fwd> >
      (new example::Fft_1024(dom, scale));
  }
};

} // namespace vsip_csl::dispatcher
} // namespace vsip_csl


// ====================================================================
// Main Program
int 
main(int argc, char **argv)
{
  // Initialize the Sourcery VSIPL++ library.
  vsipl init(argc, argv);

  // Define a typedef shorthand for the Fft objects we're using.
  typedef Fft<Vector, complex<float>, complex<float>, fft_fwd, by_reference> fft_type;

  // Set up some example inputs.
  Vector<complex<float> > v1(1024, 1.);
  Vector<complex<float> > w1(1024);
  Vector<complex<float> > v2(2048, 1.);
  Vector<complex<float> > w2(2048);

  // Compute a 1024-element FFT, which will use our custom Fft backend.
  std::cout << "Creating 1024-element Fft object" << std::endl;
  fft_type fft1(1024, 1.);
  std::cout << "Using 1024-element Fft object (two arguments)" << std::endl;
  fft1(v1, w1);
  std::cout << "Using 1024-element Fft object (one argument)" << std::endl;
  fft1(v1);

  // Compute a 2048-element FFT, which will use the system Fft backend.
  std::cout << "Creating 2048-element Fft object" << std::endl;
  fft_type fft2(2048, 1.);
  std::cout << "Using 2048-element Fft object (two arguments)" << std::endl;
  fft2(v2, w2);
  std::cout << "Using 2048-element Fft object (one argument)" << std::endl;
  fft2(v2);
}
