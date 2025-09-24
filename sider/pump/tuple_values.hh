//
//
//

#ifndef PUMP_TUPLE_VALUES_HH
#define PUMP_TUPLE_VALUES_HH
namespace pump {
    template <typename ...value_t>
    struct
    tuple_values {
        std::tuple<value_t...> values;
        tuple_values(value_t&& ...v) : values(__fwd__(v)...){}
        tuple_values(tuple_values&& rhs) : values(__fwd__(rhs.values)){}
        tuple_values(const tuple_values& rhs) : values(rhs.values){}
    };
}
#endif //PUMP_TUPLE_VALUES_HH
