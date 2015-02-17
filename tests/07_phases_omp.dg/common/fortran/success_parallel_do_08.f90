! <testinfo>
! test_generator=config/mercurium-omp
! </testinfo>
PROGRAM MAIN
    IMPLICIT NONE
    LOGICAL, ALLOCATABLE :: THREAD_FLAGS(:), OK
    INTEGER :: NTHREADS
    INTEGER :: I
    INTEGER, EXTERNAL :: OMP_GET_MAX_THREADS, OMP_GET_THREAD_NUM

    NTHREADS = OMP_GET_MAX_THREADS()
    ALLOCATE( THREAD_FLAGS(NTHREADS))

    THREAD_FLAGS = .FALSE.
    !$OMP PARALLELDO SHARED(THREAD_FLAGS)
    DO I = 1, 1000
        THREAD_FLAGS(1 + OMP_GET_THREAD_NUM()) = .TRUE.
    END DO
    !$OMP END PARALLELDO

    OK = .TRUE.
    IF (.NOT. ALL(THREAD_FLAGS(:))) THEN
        OK = .FALSE.
    END IF
    IF (.NOT. OK) STOP 1
END PROGRAM MAIN
