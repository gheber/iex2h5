/*
 *   ALL RIGHTS RESERVED.
 *   _________________________________________________________________________________
 *   NOTICE: All information contained  herein is, and remains the property  of  Varga
 *   Consulting and  its suppliers, if  any. The intellectual and  technical  concepts
 *   contained herein are proprietary to Varga Consulting and its suppliers and may be
 *   covered  by  Canadian and  Foreign Patents, patents in process, and are protected
 *   by  trade secret or copyright law. Dissemination of this information or reproduc-
 *   tion  of  this  material is strictly forbidden unless prior written permission is
 *   obtained from Varga Consulting.
 *
 *   Copyright © <2017-2025> Varga Consulting, Toronto, On     info@vargaconsulting.ca
 *   _________________________________________________________________________________
 */



namespace ch = std::chrono;
template <typename duration> inline  std::vector<duration> sequence(
		duration begin, duration interval, duration end  ){

	std::vector<duration> idx;
	duration it = begin+interval;
	while( it <= end )
		idx.push_back( it ), it += interval;

	return idx;
}

template <int P, typename T> bool is_finite( T v ){
	return  ( std::ceil( pow(10,P) * v ) /  pow(10,P) > 0.01);
}


template <typename T> arma::uvec _find_finite( const arma::Mat<T>& Q ){
	 arma::uvec idx(Q.n_rows);
	int i=0,j=0;
	do{
		if( is_finite<4>( Q(i,i)  ))
			idx[j++] = i;
	} while( ++i < Q.n_rows );
	idx.resize(j);

	return idx;
}

template <typename Duration>
Duration string2duration(const std::string& input, const std::string& fmt) {
    int h, m, s;
    char sep1, sep2;
    std::istringstream in(input);
    in >> h >> sep1 >> m >> sep2 >> s;
    if (!in || sep1 != ':' || sep2 != ':') {
        throw std::runtime_error("Invalid time format: " + input);
    }
    using namespace std::chrono;
    return duration_cast<Duration>(hours(h) + minutes(m) + seconds(s));
}

template <typename Duration>
std::string duration2string(const Duration& data, const std::string& fmt) {
    std::ostringstream out;
    out << date::format(fmt, date::make_time(data));
    return out.str();
}
