// KRATOS___
//     //   ) )
//    //         ___      ___
//   //  ____  //___) ) //   ) )
//  //    / / //       //   / /
// ((____/ / ((____   ((___/ /  MECHANICS
//
//  License:         geo_mechanics_application/license.txt
//
//  Main authors:    Vahid Galavi
//

#pragma once

#include "includes/kratos_flags.h"
#include "includes/kratos_parameters.h"
#include "processes/process.h"

#include "geo_mechanics_application_variables.h"

namespace Kratos
{

class ApplyConstantBoundaryHydrostaticPressureProcess : public Process
{

public:

    KRATOS_CLASS_POINTER_DEFINITION(ApplyConstantBoundaryHydrostaticPressureProcess);

    ApplyConstantBoundaryHydrostaticPressureProcess(ModelPart& model_part,
                                                    Parameters rParameters
                                                    ) : Process(Flags()) , mrModelPart(model_part)
    {
        KRATOS_TRY

        //only include validation with c++11 since raw_literals do not exist in c++03
        Parameters default_parameters( R"(
            {
                "model_part_name":"PLEASE_CHOOSE_MODEL_PART_NAME",
                "variable_name": "PLEASE_PRESCRIBE_VARIABLE_NAME",
                "is_fixed": false,
                "gravity_direction" : 2,
                "reference_coordinate" : 0.0,
                "specific_weight" : 10000.0,
                "table" : 1
            }  )" );

        // Some values need to be mandatorily prescribed since no meaningful default value exist. For this reason try accessing to them
        // So that an error is thrown if they don't exist
        rParameters["reference_coordinate"];
        rParameters["variable_name"];
        rParameters["model_part_name"];

        mIsFixedProvided = rParameters.Has("is_fixed");

        // Now validate against defaults -- this also ensures no type mismatch
        rParameters.ValidateAndAssignDefaults(default_parameters);

        mVariableName        = rParameters["variable_name"].GetString();
        mIsFixed             = rParameters["is_fixed"].GetBool();
        mGravityDirection    = rParameters["gravity_direction"].GetInt();
        mReferenceCoordinate = rParameters["reference_coordinate"].GetDouble();
        mSpecificWeight      = rParameters["specific_weight"].GetDouble();

        KRATOS_CATCH("")
    }


    ApplyConstantBoundaryHydrostaticPressureProcess(const ApplyConstantBoundaryHydrostaticPressureProcess&) = delete;
    ApplyConstantBoundaryHydrostaticPressureProcess& operator=(const ApplyConstantBoundaryHydrostaticPressureProcess&) = delete;
    ~ApplyConstantBoundaryHydrostaticPressureProcess() override = default;

    /// this function is designed for being called at the beginning of the computations
    /// right after reading the model and the groups
    void ExecuteInitialize() override
    {
        KRATOS_TRY

        const Variable<double> &var = KratosComponents< Variable<double> >::Get(mVariableName);

        block_for_each(mrModelPart.Nodes(), [&var, this](Node& rNode) {
            if (mIsFixed) rNode.Fix(var);
            else if (mIsFixedProvided) rNode.Free(var);

            const double pressure = mSpecificWeight * (mReferenceCoordinate - rNode.Coordinates()[mGravityDirection]);
            rNode.FastGetSolutionStepValue(var) = std::max(pressure, 0.);
        });

        KRATOS_CATCH("")
    }

    /// Turn back information as a string.
    std::string Info() const override
    {
        return "ApplyConstantBoundaryHydrostaticPressureProcess";
    }

protected:
    /// Member Variables
    ModelPart& mrModelPart;
    std::string mVariableName;
    bool mIsFixed;
    bool mIsFixedProvided;
    unsigned int mGravityDirection;
    double mReferenceCoordinate;
    double mSpecificWeight;
};

}