/******************************************************************************
** Copyright (c) 2015, Intel Corporation                                     **
** All rights reserved.                                                      **
**                                                                           **
** Redistribution and use in source and binary forms, with or without        **
** modification, are permitted provided that the following conditions        **
** are met:                                                                  **
** 1. Redistributions of source code must retain the above copyright         **
**    notice, this list of conditions and the following disclaimer.          **
** 2. Redistributions in binary form must reproduce the above copyright      **
**    notice, this list of conditions and the following disclaimer in the    **
**    documentation and/or other materials provided with the distribution.   **
** 3. Neither the name of the copyright holder nor the names of its          **
**    contributors may be used to endorse or promote products derived        **
**    from this software without specific prior written permission.          **
**                                                                           **
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       **
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         **
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     **
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      **
** HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    **
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  **
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    **
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* ******************************************************************************/
/* Narayanan Sundaram (Intel Corp.)
 * ******************************************************************************/
#include "GraphMatRuntime.cpp"

#include "Degree.cpp"

class PR {
  public:
    double pagerank;
    int degree;
  public:
    PR() {
      pagerank = 0.15;
      degree = 0;
    }
    int operator!=(const PR& p) {
      return (fabs(p.pagerank-pagerank)>1e-8);
    }
};

class PageRank : public GraphProgram<double, double, PR> {
  public:
    double alpha;

  public:

  PageRank(double a=0.15) {
    alpha = a;
    this->activity = ALL_VERTICES;
  }

  void reduce_function(double& a, const double& b) const {
    a += b;
  }
  void process_message(const double& message, const int edge_val, const PR& vertexprop, double& res) const {
    res = message;
  }
  bool send_message(const PR& vertexprop, double& message) const {
    if (vertexprop.degree == 0) {
      message = 0.0;
    } else {
      message = vertexprop.pagerank/(double)vertexprop.degree;
    }
    return true;
  }
  void apply(const double& message_out, PR& vertexprop) {
    vertexprop.pagerank = alpha + (1.0-alpha)*message_out; //non-delta update
  }

};


void run_pagerank(const char* filename, int nthreads) {

  Graph<PR> G;
  PageRank pr;
  Degree<PR> dg;

 
  G.ReadMTX(filename, nthreads*4); //nthread pieces of matrix

  auto dg_tmp = graph_program_init(dg, G);

  struct timeval start, end;
  gettimeofday(&start, 0);

  G.setAllActive();
  run_graph_program(&dg, G, 1, &dg_tmp);

  gettimeofday(&end, 0);
  double time = (end.tv_sec-start.tv_sec)*1e3+(end.tv_usec-start.tv_usec)*1e-3;
  printf("Degree Time = %.3f ms \n", time);

  graph_program_clear(dg_tmp);
  
  auto pr_tmp = graph_program_init(pr, G);

  gettimeofday(&start, 0);

  G.setAllActive();
  run_graph_program(&pr, G, -1, &pr_tmp);
  
  gettimeofday(&end, 0);
  time = (end.tv_sec-start.tv_sec)*1e3+(end.tv_usec-start.tv_usec)*1e-3;
  printf("PR Time = %.3f ms \n", time);

  graph_program_clear(pr_tmp);

  for (int i = 0; i < std::min((unsigned long long int)25, (unsigned long long int)G.getNumberOfVertices()); i++) { 
    printf("%d : %d %f\n", i, G.getVertexproperty(i).degree, G.getVertexproperty(i).pagerank);
  }

  double pr_sum = 0;
  for (int i = 0; i < (unsigned long long int)G.getNumberOfVertices(); i++) { 
    pr_sum += G.getVertexproperty(i).pagerank;
  }
  printf("sum of ranks: %f\n", pr_sum );

}

int main(int argc, char* argv[]) {

  const char* input_filename = argv[1];

  if (argc < 2) {
    printf("Correct format: %s A.mtx\n", argv[0]);
    return 0;
  }

#pragma omp parallel
  {
#pragma omp single
    {
      nthreads = omp_get_num_threads();
      printf("num threads got: %d\n", nthreads);
    }
  }
  
  run_pagerank(input_filename, nthreads);

  
}

