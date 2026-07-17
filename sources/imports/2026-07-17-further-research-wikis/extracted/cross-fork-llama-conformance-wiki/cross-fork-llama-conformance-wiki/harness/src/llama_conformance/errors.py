class ConformanceError(Exception):
    """Base harness error."""

class UncalibratedToleranceError(ConformanceError):
    """Raised when a numeric/distributional comparison lacks an approved profile."""

class InvalidObservationError(ConformanceError):
    """Raised when an observation lacks required provenance or output structure."""
