! <testinfo>
! test_generator=(config/mercurium-ompss "config/mercurium-ompss-v2 openmp-compatibility")
! </testinfo>
MODULE MOD_TYPES
    IMPLICIT NONE

    TYPE, PUBLIC :: MY_TYPE
        INTEGER :: X
    END TYPE MY_TYPE
END MODULE MOD_TYPES

MODULE M
    IMPLICIT NONE
    CONTAINS
        SUBROUTINE FOO(VAR)
            USE MOD_TYPES
            IMPLICIT NONE
            TYPE(MY_TYPE) :: VAR

            !$OMP TASK FIRSTPRIVATE(VAR)
                VAR % X = 1
            !$OMP END TASK

            !$OMP TASKWAIT
        END SUBROUTINE FOO
END MODULE M


PROGRAM P
    USE M
    USE MOD_TYPES
    IMPLICIT NONE
    TYPE(MY_TYPE) :: VAR
    VAR % X = -1
    CALL FOO(VAR)

    IF (VAR % X /= -1) STOP 1
END PROGRAM P
