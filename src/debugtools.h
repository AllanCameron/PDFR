/*---------------------------------------------------------------------------*/
/* PDFR debugging tools                                                           */
/*---------------------------------------------------------------------------*/

template<typename T>
void printvec(std::vector<T> x)
{
  size_t s = x.size();
  if(s == 0) std::cout << "[empty vector]" << std::endl;
  if(s > 1) for(size_t i = 0; i < (s - 1); i++) std::cout << x[i] << ", ";
  if(s > 0) std::cout << x[s - 1] << std::endl;
}

/*---------------------------------------------------------------------------*/

inline std::chrono::high_resolution_clock::time_point startClock()
{
  return std::chrono::high_resolution_clock::now();
}

/*---------------------------------------------------------------------------*/

inline void timeSince(std::string message,
               std::chrono::high_resolution_clock::time_point& start)
{
  std::chrono::high_resolution_clock::time_point end =
    std::chrono::high_resolution_clock::now();
  auto diff = end - start;
  std::cout << message << ": "
            << std::chrono::duration <double, std::milli> (diff).count()
            << " ms" << std::endl;
  start = std::chrono::high_resolution_clock::now();
}

/*---------------------------------------------------------------------------*/
