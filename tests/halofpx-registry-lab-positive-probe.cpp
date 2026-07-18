namespace halofpx {
struct concrete_registry_lab_observation {};
void concrete_registry_lab_observation_symbol() {}
}

int main() {
    halofpx::concrete_registry_lab_observation observation {};
    halofpx::concrete_registry_lab_observation_symbol();
    (void) observation;
    return 0;
}
