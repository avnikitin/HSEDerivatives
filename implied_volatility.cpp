#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

typedef long double ld;

class BSMmodel {
private:
  ld StdNormCDF(ld x) {
    // returns P(Y <= x) where Y ~ N(0, 1)
    return erfc(-x / sqrt(2)) / 2;
  }

  ld Time; // to maturity
  ld Spot;
  ld Strike;
  ld r;     // risk free rate
  ld Sigma; // volatility

  ld call;
  ld put;

  ld profit(ld x, char type) {
    assert(type == 'c' || type == 'p');
    if (type == 'c') {
      return std::max((ld)0, x - Strike);
    }
    return std::max(Strike - x, (ld)0);
  }

  void calculatePremiums(int simulations_num = 10000, int time_nums = 100) {
    // Assuming we might expire options at times {0, dt, 2dt, ..., Time}
    ld dt = Time / time_nums;
    std::vector<ld> spots(simulations_num, Spot);
    std::vector<ld> call_profits(simulations_num, 0);
    std::vector<ld> put_profits(simulations_num, 0);
    ld best_call_profit = 0; // if we expire options at time 0
    ld best_put_profit = 0;

    std::random_device rd{};
    std::mt19937 gen{rd()};

    for (int time = 1; time <= time_nums; time++) {
      for (int i = 0; i < simulations_num; i++) {
        ld prev_price = spots[i];

        std::normal_distribution<> d{
            log(prev_price) + (r - Sigma * Sigma / 2) * dt, Sigma * sqrt(dt)};
        ld log_new_price = d(gen);
        spots[i] = exp(log_new_price);
        call_profits[i] = profit(spots[i], 'c');
        put_profits[i] = profit(spots[i], 'p');
        // std::cout << spots[i] << ' ';
      }
      // std::cout << '\n';
      ld call_average =
          std::accumulate(call_profits.begin(), call_profits.end(), 0.0) /
          simulations_num;
      ld put_average =
          std::accumulate(put_profits.begin(), put_profits.end(), 0.0) /
          simulations_num;
      best_call_profit = std::max(best_call_profit, call_average);
      best_put_profit = std::max(best_put_profit, put_average);
    }

    call = best_call_profit;
    put = best_put_profit;
  }

public:
  BSMmodel(ld _Time, ld _Spot, ld _Strike, ld _r, ld _Sigma)
      : Time(_Time), Spot(_Spot), Strike(_Strike), r(_r), Sigma(_Sigma) {
    calculatePremiums();
  }

  ld get_price(char type) {
    assert(type == 'c' || type == 'p');
    if (type == 'c') {
      return call;
    }
    return put;
  }
};

ld calculateImpliedVolatility(ld Time, ld Spot, ld Strike, ld r, char type,
                              ld Premium, ld tol = 1e-5) {
  ld low_vol = 0.03,
     high_vol =
         6; // lowest and highest possible volatility. Taken from
  // https://www.barchart.com/options/highest-implied-volatility/highest

  while (high_vol - low_vol > tol) {
    ld mid_vol = (low_vol + high_vol) / 2;
    BSMmodel model(Time, Spot, Strike, r, mid_vol);
    if (model.get_price(type) > Premium) {
      high_vol = mid_vol;
    } else if (model.get_price(type) < Premium) {
      low_vol = mid_vol;
    } else {
      return mid_vol; // It's quite unlikely
    }
  }
  return low_vol;
}

int main() {
  //                                       Time    Spot   K    r   type   price
  std::cout << calculateImpliedVolatility(0.0493, 75.576, 75, 0.08, 'p', 1.298) * 100 << '%';
}
