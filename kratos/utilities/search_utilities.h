//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:         BSD License
//                   Kratos default license: kratos/license.txt
//
//  Main authors:    Philipp Bucher
//                   Vicente Mataix Ferrandiz

#pragma once

// System includes
#include <vector>
#include <numeric>

// External includes

// Project includes
#include "geometries/bounding_box.h"
#include "geometries/point.h"
#include "includes/data_communicator.h"
#include "includes/variables.h"
#include "includes/model_part.h"
#include "spatial_containers/point_object.h"
#include "utilities/parallel_utilities.h"

namespace Kratos
{
///@name Kratos Globals
///@{

///@}
///@name Type Definitions
///@{

///@}
///@name  Enum's
///@{

///@}
///@name  Functions
///@{

///@}
///@name Kratos Classes
///@{

/**
 * @brief This struct provides information related with distributed search
 * @details The class contains the following:
 *  - PointCoordinates: The coordinates of the points to be created
 *  - Indexes: The indexes of the points to be created
 *  - Ranks: The ranks where the results will be defined
 */
struct DistributedSearchInformation
{
    /// Alias for the data type used to represent indices.
    using IndexType = std::size_t;

    /// Alias for the data type used to represent sizes.
    using SizeType = std::size_t;

    std::vector<double> PointCoordinates; /// Vector to store point coordinates.
    std::vector<IndexType> Indexes;       /// Vector to store point indices.
    std::vector<int> SearchRanks;         /// Ranks where search is invoked.
    std::vector<std::vector<int>> Ranks;  /// Vector of vectors to store ranks.

    /**
     * @brief Reserve memory for the point data vectors.
     * @param Size The size to reserve.
     */
    void Reserve(const SizeType Size)
    {
        PointCoordinates.reserve(Size * 3);
        Indexes.reserve(Size);
        SearchRanks.reserve(Size);
        Ranks.reserve(Size);
    }

    /**
     * @brief Shrink the capacity of the point data vectors to fit the data.
     */
    void Shrink()
    {
        PointCoordinates.shrink_to_fit();
        Indexes.shrink_to_fit();
        SearchRanks.shrink_to_fit();
        Ranks.shrink_to_fit();
    }

    /**
     * @brief Clear all the data in the point data vectors.
     */
    void Clear()
    {
        PointCoordinates.clear();
        Indexes.clear();
        SearchRanks.clear();
        Ranks.clear();
    }
};

/**
 * @class SearchUtilities
 * @ingroup KratosCore
 * @brief MPI utilities for searching geometrical objects
 * @details Some methods original implementation coming from MappingUtilities
 * @author Philipp Bucher (MappingUtilities methods)
 * @author Vicente Mataix Ferrandiz
 */
class SearchUtilities
{
public:
    ///@name Type Definitions
    ///@{

    /// The Bounding Box type
    using BoundingBoxType = std::array<double, 6>;

    /// The index type definition
    using IndexType = long unsigned int;

    /// The size type definition
    using SizeType = std::size_t;

    /// Input/output types
    using RadiusArrayType = std::vector<double>;
    using DistanceType = std::vector<double>;
    using VectorDistanceType = std::vector<DistanceType>;

    /// Define zero tolerance as Epsilon
    static constexpr double ZeroTolerance = std::numeric_limits<double>::epsilon();

    ///@}
    ///@name Life Cycle
    ///@{

    ///@}
    ///@name Operations
    ///@{

    /**
     * @brief Check if a point is inside a bounding box
     * @details Bounding box class implementation
     * @param rBoundingBox The bounding box
     * @param rCoords The point
     * @return true if the point is inside the bounding box
     * @tparam TPointType The type of point considered
     */
    template<class TPointType>
    static bool PointIsInsideBoundingBox(
        const BoundingBox<TPointType>& rBoundingBox,
        const array_1d<double, 3>& rCoords
        )
    {
        // Get the bounding box points
        const auto& r_max_point = rBoundingBox.GetMaxPoint();
        const auto& r_min_point = rBoundingBox.GetMinPoint();

        // The Bounding Box check
        if (rCoords[0] < r_max_point[0] && rCoords[0] > r_min_point[0])           // check x-direction
            if (rCoords[1] < r_max_point[1] && rCoords[1] > r_min_point[1])       // check y-direction
                if (rCoords[2] < r_max_point[2] && rCoords[2] > r_min_point[2])   // check z-direction
                    return true;
        return false;
    }

    /**
     * @brief Check if a point is inside a bounding box
     * @details Bounding box array of 6 doubles implementation
     * @param rBoundingBox The bounding box
     * @param rCoords The point
     * @return true if the point is inside the bounding box
     */
    static bool PointIsInsideBoundingBox(
        const BoundingBoxType& rBoundingBox,
        const array_1d<double, 3>& rCoords
        )
    {
        // The Bounding Box should have some tolerance already!
        if (rCoords[0] < rBoundingBox[0] && rCoords[0] > rBoundingBox[1])           // check x-direction
            if (rCoords[1] < rBoundingBox[2] && rCoords[1] > rBoundingBox[3])       // check y-direction
                if (rCoords[2] < rBoundingBox[4] && rCoords[2] > rBoundingBox[5])   // check z-direction
                    return true;
        return false;
    }

    /**
     * @brief This method checks if a point is inside a bounding box considering a certain tolerance
     * @param rBoundingBox The bounding box
     * @param rCoords The coordinates of the point
     * @param Tolerance The tolerance
     * @return True if the point is inside the bounding box
     * @tparam TPointType The type of point considered
     */
    template<class TPointType>
    static bool PointIsInsideBoundingBox(
        const BoundingBox<TPointType>& rBoundingBox,
        const array_1d<double, 3>& rCoords,
        const double Tolerance
        )
    {
        // Get the bounding box points
        auto max_point = rBoundingBox.GetMaxPoint();
        auto min_point = rBoundingBox.GetMinPoint();

        // Apply Tolerances (only in non zero BB cases)
        const double epsilon = std::numeric_limits<double>::epsilon();
        if (norm_2(max_point) > epsilon && norm_2(min_point) > epsilon) {
            for (unsigned int i=0; i<3; ++i) {
                max_point[i] += Tolerance;
                min_point[i] -= Tolerance;
            }
        }

        // The Bounding Box check
        if (rCoords[0] < max_point[0] && rCoords[0] > min_point[0])           // check x-direction
            if (rCoords[1] < max_point[1] && rCoords[1] > min_point[1])       // check y-direction
                if (rCoords[2] < max_point[2] && rCoords[2] > min_point[2])   // check z-direction
                    return true;
        return false;
    }

    /**
     * @brief Compute the bounding boxes of the given bounding boxes from a given tolerance
     * @param rBoundingBoxes The bounding boxes
     * @param Tolerance The tolerance
     * @param rBoundingBoxesWithTolerance The resulting bounding boxes with the applied tolerance
     */
    static void ComputeBoundingBoxesWithTolerance(
        const std::vector<double>& rBoundingBoxes,
        const double Tolerance,
        std::vector<double>& rBoundingBoxesWithTolerance
        );

    /**
     * @brief Compute the bounding boxes of the given bounding boxes from a given tolerance, additionally checking if the bounding boxes are initialized
     * @details This method is used when the bounding boxes are not initialized
     * @param rBoundingBoxes The bounding boxes
     * @param Tolerance The tolerance
     * @param rBoundingBoxesWithTolerance The resulting bounding boxes with the applied tolerance
     */
    static void ComputeBoundingBoxesWithToleranceCheckingNullBB(
        const std::vector<double>& rBoundingBoxes,
        const double Tolerance,
        std::vector<double>& rBoundingBoxesWithTolerance
        );

    /**
     * @brief SynchronousPointSynchronization prepares synchronously the coordinates of the points for MPI search.
     * @param itPointBegin Iterator to the beginning of the points range
     * @param itPointEnd Iterator to the end of the points range
     * @param rAllPointsCoordinates vector where the computed coordinates will be stored
     * @param rAllPointsIds The ids of all the points (just a counter for points, and ids for nodes)
     * @param rDataCommunicator The data communicator
     * @tparam TPointIteratorType The type of the point iterator
     */
    template<typename TPointIteratorType>
    static void SynchronousPointSynchronization(
        TPointIteratorType itPointBegin,
        TPointIteratorType itPointEnd,
        std::vector<double>& rAllPointsCoordinates,
        std::vector<IndexType>& rAllPointsIds,
        const DataCommunicator& rDataCommunicator
        )
    {
        // First check that the points are the same in all processes
        int number_of_points, total_number_of_points;
        CalculateNumberOfPoints(itPointBegin, itPointEnd, number_of_points, total_number_of_points, rDataCommunicator);

        KRATOS_DEBUG_ERROR_IF(number_of_points < 0) << "The number of points is negative" << std::endl;
        KRATOS_DEBUG_ERROR_IF(total_number_of_points < 0) << "The total number of points is negative" << std::endl;

        // We synchronize the points
        SynchronizePoints(itPointBegin, itPointEnd, rAllPointsCoordinates, rAllPointsIds, rDataCommunicator, number_of_points, total_number_of_points);
    }

    /**
     * @brief SynchronousPointSynchronization prepares synchronously the coordinates of the points for MPI search.
     * @param itPointBegin Iterator to the beginning of the points range.
     * @param itPointEnd Iterator to the end of the points range.
     * @param rSearchInfo The class containing the result of the search.
     * @param rBoundingBox The bounding box considered.
     * @param ThresholdBoundingBox The threshold for computing is inside bounding box considered.
     * @param rDataCommunicator The data communicator.
     * @param IndexItIsJustCounter If the index considered it it just a counter.
     * @tparam TPointIteratorType The type of the point iterator.
     * @tparam TBoundingBoxType The type of the bounding box.
     * @return The ids of all points.
     */
    template<typename TPointIteratorType, typename TBoundingBoxType>
    static std::vector<IndexType> SynchronousPointSynchronizationWithBoundingBox(
        TPointIteratorType itPointBegin,
        TPointIteratorType itPointEnd,
        DistributedSearchInformation& rSearchInfo,
        const TBoundingBoxType& rBoundingBox,
        const double ThresholdBoundingBox,
        const DataCommunicator& rDataCommunicator,
        const bool IndexItIsJustCounter = false
        )
    {
        // First check that the points are the same in all processes
        int number_of_points, total_number_of_points;
        CalculateNumberOfPoints(itPointBegin, itPointEnd, number_of_points, total_number_of_points, rDataCommunicator);

        KRATOS_DEBUG_ERROR_IF(number_of_points < 0) << "The number of points is negative" << std::endl;
        KRATOS_DEBUG_ERROR_IF(total_number_of_points < 0) << "The total number of points is negative" << std::endl;

        // We synchronize the points
        return SynchronizePointsWithBoundingBox(itPointBegin, itPointEnd, rSearchInfo, rBoundingBox, ThresholdBoundingBox, rDataCommunicator, number_of_points, total_number_of_points, IndexItIsJustCounter);
    }

    /**
     * @brief SynchronousPointSynchronizationWithRecvSizes prepares synchronously the coordinates of the points for MPI search including the recv sizes
     * @details With recv sizes
     * @param itPointBegin Iterator to the beginning of the points range
     * @param itPointEnd Iterator to the end of the points range
     * @param rAllPointsCoordinates vector where the computed coordinates will be stored
     * @param rAllPointsIds The ids of all the points (just a counter for points, and ids for nodes)
     * @param rDataCommunicator The data communicator
     * @return The resulting whole radius vector
     * @tparam TPointIteratorType The type of the point iterator
     */
    template<typename TPointIteratorType>
    static std::vector<int> SynchronousPointSynchronizationWithRecvSizes(
        TPointIteratorType itPointBegin,
        TPointIteratorType itPointEnd,
        std::vector<double>& rAllPointsCoordinates,
        std::vector<IndexType>& rAllPointsIds,
        const DataCommunicator& rDataCommunicator
        )
    {
        // First check that the points are the same in all processes
        int number_of_points, total_number_of_points;
        CalculateNumberOfPoints(itPointBegin, itPointEnd, number_of_points, total_number_of_points, rDataCommunicator);

        KRATOS_DEBUG_ERROR_IF(number_of_points < 0) << "The number of points is negative" << std::endl;
        KRATOS_DEBUG_ERROR_IF(total_number_of_points < 0) << "The total number of points is negative" << std::endl;

        // We synchronize the points
        return SynchronizePoints(itPointBegin, itPointEnd, rAllPointsCoordinates, rAllPointsIds, rDataCommunicator, number_of_points, total_number_of_points);
    }

    /**
     * @brief SynchronousPointSynchronizationWithRadius prepares synchronously the coordinates of the points for MPI search including radius
     * @details With radius
     * @param itPointBegin Iterator to the beginning of the points range
     * @param itPointEnd Iterator to the end of the points range
     * @param rAllPointsCoordinates vector where the computed coordinates will be stored
     * @param rAllPointsIds The ids of all the points (just a counter for points, and ids for nodes)
     * @param rRadius The radius of the points
     * @param rDataCommunicator The data communicator
     * @return The resulting whole radius vector
     * @tparam TPointIteratorType The type of the point iterator
     */
    template<typename TPointIteratorType>
    static std::vector<double> SynchronousPointSynchronizationWithRadius(
        TPointIteratorType itPointBegin,
        TPointIteratorType itPointEnd,
        std::vector<double>& rAllPointsCoordinates,
        std::vector<IndexType>& rAllPointsIds,
        const std::vector<double>& rRadius,
        const DataCommunicator& rDataCommunicator
        )
    {
        // First check that the points are the same in all processes
        int number_of_points, total_number_of_points;
        CalculateNumberOfPoints(itPointBegin, itPointEnd, number_of_points, total_number_of_points, rDataCommunicator);

        KRATOS_DEBUG_ERROR_IF(number_of_points < 0) << "The number of points is negative" << std::endl;
        KRATOS_DEBUG_ERROR_IF(total_number_of_points < 0) << "The total number of points is negative" << std::endl;

        // We synchronize the points
        const auto recv_sizes = SynchronizePoints(itPointBegin, itPointEnd, rAllPointsCoordinates, rAllPointsIds, rDataCommunicator, number_of_points, total_number_of_points);

        // Get radius
        if (rDataCommunicator.IsDistributed()) { // If distributed
            return SynchronizeRadius(recv_sizes, rRadius, rDataCommunicator);
        } else { // If serial
            return rRadius;
        }
    }

    /**
     * @brief This method prepares the search
     * @param rStructure The structure to be searched
     * @param rInput The input to be searched
     * @param rResults The results
     * @param rResultsDistance The results distance
     * @tparam TContainer The container type
     * @tparam TResultType The result type
     */
    template<class TContainer, class TResultType>
    static std::vector<typename PointObject<typename TContainer::value_type>::Pointer> PrepareSearch(
        const TContainer& rStructure,
        const TContainer& rInput,
        TResultType& rResults,
        VectorDistanceType& rResultsDistance
        )
    {
        // Resizing the results
        PrepareOutputSearch(rInput, rResults, rResultsDistance);

        // Preparing the points
        return PreparePointsSearch(rStructure);
    }

    /**
     * @brief This method prepares the search output
     * @param rInput The input to be searched
     * @param rResults The results
     * @param rResultsDistance The results distance
     * @tparam TContainer The container type
     * @tparam TResultType The result type
     */
    template<class TContainer, class TResultType>
    static void PrepareOutputSearch(
        const TContainer& rInput,
        TResultType& rResults,
        VectorDistanceType& rResultsDistance
        )
    {
        // Resizing the results
        const std::size_t input_size = rInput.size();
        if (rResults.size() != input_size) {
            rResults.resize(input_size);
        }
        if (rResultsDistance.size() != input_size) {
            rResultsDistance.resize(input_size);
        }
    }

    /**
     * @brief This method prepares the points for search
     * @param rStructure The structure to be searched
     * @tparam TContainer The container type
     */
    template<class TContainer>
    static std::vector<typename PointObject<typename TContainer::value_type>::Pointer> PreparePointsSearch(const TContainer& rStructure)
    {
        // Some definitions
        using ObjectType = typename TContainer::value_type;
        using PointType = PointObject<ObjectType>;
        using PointTypePointer = typename PointType::Pointer;
        using PointVector = std::vector<PointTypePointer>;

        // Creating the points
        PointVector points;
        const std::size_t structure_size = rStructure.size();
        points.reserve(structure_size);
        const auto it_begin = rStructure.begin();
        for (std::size_t i = 0; i < structure_size; ++i) {
            auto it = it_begin + i;
            points.push_back(PointTypePointer(new PointType(*(it.base()))));
        }

        return points;
    }

    /**
     * @brief This method performs the search in parallel
     * @param rInput The input container
     * @param rRadius The radius array
     * @param rSearch The spatial search
     * @param rResults The results
     * @param rResultsDistance The results distance
     * @param AllocationSize The allocation size
     * @tparam TContainer The container type
     * @tparam TSpatialContainer The spatial container type
     * @tparam TResultType The result type
     */
    template<class TContainer, class TSpatialContainer, class TResultType>
    static void ParallelSearch(
        const TContainer& rInput,
        const RadiusArrayType& rRadius,
        TSpatialContainer& rSearch,
        TResultType& rResults,
        VectorDistanceType& rResultsDistance,
        const int AllocationSize = 1000
        )
    {
        // Some definitions
        using ObjectType = typename TContainer::value_type;
        using PointType = PointObject<ObjectType>;
        using PointTypePointer = typename PointType::Pointer;
        using PointVector = std::vector<PointTypePointer>;
        using DistanceVector = std::vector<double>;
        const std::size_t input_size = rInput.size();

        // Performing search
        IndexPartition<std::size_t>(input_size).for_each([&](std::size_t i) {
            auto it = rInput.begin() + i;
            PointType aux_point(*(it.base()));
            PointVector results(AllocationSize);
            DistanceVector results_distances(AllocationSize);
            const std::size_t number_of_results = rSearch.SearchInRadius(aux_point, rRadius[i], results.begin(), results_distances.begin(), AllocationSize);
            if (number_of_results > 0) {
                auto& r_results = rResults[i];
                auto& r_results_distance = rResultsDistance[i];
                r_results.reserve(number_of_results);
                r_results_distance.reserve(number_of_results);
                for (std::size_t j = 0; j < number_of_results; ++j) {
                    auto p_point = results[j];
                    r_results.push_back(p_point->pGetObject());
                    r_results_distance.push_back(results_distances[j]);
                }
            }
        });
    }

    ///@}
private:
    ///@name Private Operations
    ///@{

    /**
     * @details Synchronizes points between different processes. 
     * @details Synchonously
     * @param itPointBegin Iterator pointing to the beginning of the range of points
     * @param itPointEnd Iterator pointing to the end of the range of points
     * @param rAllPointsCoordinates Vector to store the synchronized points' coordinates
     * @param rAllPointsIds The ids of all the points (just a counter for points, and ids for nodes)
     * @param rDataCommunicator Object for data communication between processes
     * @param NumberOfPoints Local number of points to be synchronized
     * @param TotalNumberOfPoints Total number of points across all processes
     * @return A vector containing the sizes of data for each process
     * @tparam TPointIteratorType The type of the point iterator
     */
    template<typename TPointIteratorType>
    static std::vector<int> SynchronizePoints(
        TPointIteratorType itPointBegin,
        TPointIteratorType itPointEnd,
        std::vector<double>& rAllPointsCoordinates,
        std::vector<IndexType>& rAllPointsIds,
        const DataCommunicator& rDataCommunicator,
        const int NumberOfPoints,
        const int TotalNumberOfPoints
        )
    {
        // Get the World Size and rank in MPI
        const int world_size = rDataCommunicator.Size();
        const int rank = rDataCommunicator.Rank();

        // Getting global number of points
        std::vector<int> points_per_partition(world_size);
        std::vector<int> send_points_per_partition(1, NumberOfPoints);
        rDataCommunicator.AllGather(send_points_per_partition, points_per_partition);
        std::size_t initial_id = 0;
        if constexpr (!std::is_same<TPointIteratorType, ModelPart::NodeIterator>::value || std::is_same<TPointIteratorType, ModelPart::NodeConstantIterator>::value) {
            initial_id = std::accumulate(points_per_partition.begin(), points_per_partition.begin() + rank, 0);
        }

        // Initialize and resize vectors
        if (rAllPointsCoordinates.size() != static_cast<unsigned int>(TotalNumberOfPoints * 3)) {
            rAllPointsCoordinates.resize(TotalNumberOfPoints * 3);
        }
        if (rAllPointsIds.size() != static_cast<unsigned int>(TotalNumberOfPoints)) {
            rAllPointsIds.resize(TotalNumberOfPoints);
        }
        std::vector<int> recv_sizes(world_size, 0);

        // Auxiliary variables
        std::size_t counter = 0;
        array_1d<double, 3> coordinates;
        unsigned int i_coord;

        // If distributed
        if (rDataCommunicator.IsDistributed()) {
            // Initialize local points coordinates
            std::vector<double> send_points_coordinates(NumberOfPoints * 3);
            std::vector<IndexType> send_points_ids(NumberOfPoints);
            for (auto it_point = itPointBegin ; it_point != itPointEnd ; ++it_point) {
                // In case of considering nodes
                if constexpr (std::is_same<TPointIteratorType, ModelPart::NodeIterator>::value || std::is_same<TPointIteratorType, ModelPart::NodeConstantIterator>::value) {
                    if (it_point->FastGetSolutionStepValue(PARTITION_INDEX) != rank) {
                        continue; // Skip if not local
                    }
                }
                noalias(coordinates) = it_point->Coordinates();
                if constexpr (std::is_same<TPointIteratorType, ModelPart::NodeIterator>::value || std::is_same<TPointIteratorType, ModelPart::NodeConstantIterator>::value) {
                    send_points_ids[counter] = it_point->Id();
                } else {
                    send_points_ids[counter] = initial_id + counter;
                }
                for (i_coord = 0; i_coord < 3; ++i_coord) {
                    send_points_coordinates[3 * counter + i_coord] = coordinates[i_coord];
                }
                ++counter;
            }

            /* Sync coordinates */

            // Generate vectors with sizes for AllGatherv
            for (int i_rank = 0; i_rank < world_size; ++i_rank) {
                recv_sizes[i_rank] = 3 * points_per_partition[i_rank];
            }
            std::vector<int> recv_offsets(world_size, 0);
            for (int i_rank = 1; i_rank < world_size; ++i_rank) {
                recv_offsets[i_rank] = recv_offsets[i_rank - 1] + recv_sizes[i_rank - 1];
            }

            // Invoke AllGatherv
            rDataCommunicator.AllGatherv(send_points_coordinates, rAllPointsCoordinates, recv_sizes, recv_offsets);

            /* Sync Ids */

            // Generate vectors with sizes for AllGatherv
            for (int i_rank = 0; i_rank < world_size; ++i_rank) {
                recv_sizes[i_rank] = points_per_partition[i_rank];
            }
            for (int i_rank = 1; i_rank < world_size; ++i_rank) {
                recv_offsets[i_rank] = recv_offsets[i_rank - 1] + recv_sizes[i_rank - 1];
            }

            // Invoke AllGatherv
            rDataCommunicator.AllGatherv(send_points_ids, rAllPointsIds, recv_sizes, recv_offsets);
        } else { // Serial
            // Assign values
            for (auto it_point = itPointBegin ; it_point != itPointEnd ; ++it_point) {
                noalias(coordinates) = it_point->Coordinates();
                if constexpr (std::is_same<TPointIteratorType, ModelPart::NodeIterator>::value || std::is_same<TPointIteratorType, ModelPart::NodeConstantIterator>::value) {
                    rAllPointsIds[counter] = it_point->Id();
                } else {
                    rAllPointsIds[counter] = initial_id + counter;
                }
                for (i_coord = 0; i_coord < 3; ++i_coord) {
                    rAllPointsCoordinates[3 * counter + i_coord] = coordinates[i_coord];
                }
                ++counter;
            }
        }

        // Return the recv sizes
        return recv_sizes;
    }

    /**
     * @details Synchronizes points between different processes.
     * @details Synchonously.
     * @param itPointBegin Iterator pointing to the beginning of the range of points.
     * @param itPointEnd Iterator pointing to the end of the range of points.
     * @param rAllPointsCoordinates Vector to store the synchronized points' coordinates.
     * @param rAllPointsIds The ids of all the points (just a counter for points, and ids for nodes).
     * @param rDataCommunicator Object for data communication between processes.
     * @param NumberOfPoints Local number of points to be synchronized.
     * @param TotalNumberOfPoints Total number of points across all processes.
     * @param IndexItIsJustCounter If the index considered it it just a counter.
     * @return A vector containing the ranks of each point.
     * @tparam TPointIteratorType The type of the point iterator.
     */
    template<typename TPointIteratorType>
    static std::vector<int> SynchronizePointsWithRanks(
        TPointIteratorType itPointBegin,
        TPointIteratorType itPointEnd,
        std::vector<double>& rAllPointsCoordinates,
        std::vector<IndexType>& rAllPointsIds,
        const DataCommunicator& rDataCommunicator,
        const int NumberOfPoints,
        const int TotalNumberOfPoints,
        const bool IndexItIsJustCounter = false
        )
    {
        // Define lambda to retrieve Id
        const auto GetIdNode = [](std::vector<IndexType>& rIds, TPointIteratorType& ItPoint, const std::size_t Counter, const std::size_t InitialId) {
            if constexpr (std::is_same<TPointIteratorType, ModelPart::NodeIterator>::value || std::is_same<TPointIteratorType, ModelPart::NodeConstantIterator>::value) {
                rIds[Counter] = ItPoint->Id();
            } else {
                rIds[Counter] = InitialId + Counter;
            }
        };
        const auto GetIdJustCounter = [](std::vector<IndexType>& rIds, TPointIteratorType& ItPoint, const std::size_t Counter, const std::size_t InitialId) {
            rIds[Counter] = Counter;
        };
        const auto GetId = IndexItIsJustCounter ? GetIdJustCounter : GetIdNode;

        // Get the World Size and rank in MPI
        const int world_size = rDataCommunicator.Size();
        const int rank = rDataCommunicator.Rank();

        // Getting global number of points
        std::vector<int> points_per_partition(world_size);
        std::vector<int> send_points_per_partition(1, NumberOfPoints);
        std::vector<int> all_points_ranks(TotalNumberOfPoints);
        rDataCommunicator.AllGather(send_points_per_partition, points_per_partition);
        std::size_t initial_id = 0;
        if constexpr (!std::is_same<TPointIteratorType, ModelPart::NodeIterator>::value || std::is_same<TPointIteratorType, ModelPart::NodeConstantIterator>::value) {
            initial_id = std::accumulate(points_per_partition.begin(), points_per_partition.begin() + rank, 0);
        }

        // Initialize and resize vectors
        if (rAllPointsCoordinates.size() != static_cast<std::size_t>(TotalNumberOfPoints * 3)) {
            rAllPointsCoordinates.resize(TotalNumberOfPoints * 3);
        }
        if (rAllPointsIds.size() != static_cast<std::size_t>(TotalNumberOfPoints)) {
            rAllPointsIds.resize(TotalNumberOfPoints);
        }
        std::vector<int> recv_sizes(world_size, 0);

        // Auxiliary variables
        std::size_t counter = 0;
        unsigned int i_coord;

        // If distributed
        if (rDataCommunicator.IsDistributed()) {
            // Initialize local points coordinates
            std::vector<double> send_points_coordinates(NumberOfPoints * 3);
            std::vector<IndexType> send_points_ids(NumberOfPoints);
            std::vector<int> send_points_ranks(NumberOfPoints);
            for (auto it_point = itPointBegin ; it_point != itPointEnd ; ++it_point) {
                // In case of considering nodes
                if constexpr (std::is_same<TPointIteratorType, ModelPart::NodeIterator>::value || std::is_same<TPointIteratorType, ModelPart::NodeConstantIterator>::value) {
                    if (it_point->FastGetSolutionStepValue(PARTITION_INDEX) != rank) {
                        continue; // Skip if not local
                    }
                }
                const auto& r_coordinates = it_point->Coordinates();
                GetId(send_points_ids, it_point, counter, initial_id);
                send_points_ranks[counter] = rank;
                for (i_coord = 0; i_coord < 3; ++i_coord) {
                    send_points_coordinates[3 * counter + i_coord] = r_coordinates[i_coord];
                }
                ++counter;
            }

            /* Sync coordinates */

            // Generate vectors with sizes for AllGatherv
            for (int i_rank = 0; i_rank < world_size; ++i_rank) {
                recv_sizes[i_rank] = 3 * points_per_partition[i_rank];
            }
            std::vector<int> recv_offsets(world_size, 0);
            for (int i_rank = 1; i_rank < world_size; ++i_rank) {
                recv_offsets[i_rank] = recv_offsets[i_rank - 1] + recv_sizes[i_rank - 1];
            }

            // Invoke AllGatherv
            rDataCommunicator.AllGatherv(send_points_coordinates, rAllPointsCoordinates, recv_sizes, recv_offsets);

            /* Sync Ids */

            // Generate vectors with sizes for AllGatherv
            for (int i_rank = 0; i_rank < world_size; ++i_rank) {
                recv_sizes[i_rank] = points_per_partition[i_rank];
            }
            for (int i_rank = 1; i_rank < world_size; ++i_rank) {
                recv_offsets[i_rank] = recv_offsets[i_rank - 1] + recv_sizes[i_rank - 1];
            }

            // Invoke AllGatherv
            rDataCommunicator.AllGatherv(send_points_ids, rAllPointsIds, recv_sizes, recv_offsets);
            rDataCommunicator.AllGatherv(send_points_ranks, all_points_ranks, recv_sizes, recv_offsets);
        } else { // Serial
            // Assign values
            for (auto it_point = itPointBegin ; it_point != itPointEnd ; ++it_point) {
                const auto& r_coordinates = it_point->Coordinates();
                if constexpr (std::is_same<TPointIteratorType, ModelPart::NodeIterator>::value || std::is_same<TPointIteratorType, ModelPart::NodeConstantIterator>::value) {
                    rAllPointsIds[counter] = it_point->Id();
                } else {
                    rAllPointsIds[counter] = initial_id + counter;
                }
                for (i_coord = 0; i_coord < 3; ++i_coord) {
                    rAllPointsCoordinates[3 * counter + i_coord] = r_coordinates[i_coord];
                }
                ++counter;
            }
        }

        // Return the ranks
        return all_points_ranks;
    }

    /**
     * @brief Synchronizes points between different processes.
     * @details Synchonously.
     * @param itPointBegin Iterator pointing to the beginning of the range of points.
     * @param itPointEnd Iterator pointing to the end of the range of points.
     * @param rSearchInfo The class containing the result of the search.
     * @param rBoundingBox The bounding box considered.
     * @param ThresholdBoundingBox The threshold for computing is inside bounding box considered.
     * @param rDataCommunicator Object for data communication between processes.
     * @param NumberOfPoints Local number of points to be synchronized.
     * @param TotalNumberOfPoints Total number of points across all processes.
     * @param IndexItIsJustCounter If the index considered it it just a counter.
     * @return A vector containing the sizes of data for each process.
     * @tparam TPointIteratorType The type of the point iterator.
     * @tparam TBoundingBoxType The type of the bounding box.
     * @return The ids of all points.
     */
    template<typename TPointIteratorType, typename TBoundingBoxType>
    static std::vector<IndexType> SynchronizePointsWithBoundingBox(
        TPointIteratorType itPointBegin,
        TPointIteratorType itPointEnd,
        DistributedSearchInformation& rSearchInfo,
        const TBoundingBoxType& rBoundingBox,
        const double ThresholdBoundingBox,
        const DataCommunicator& rDataCommunicator,
        const int NumberOfPoints,
        const int TotalNumberOfPoints,
        const bool IndexItIsJustCounter = false
        )
    {
        // Initialize and resize vectors
        rSearchInfo.Reserve(TotalNumberOfPoints);
        std::vector<double> all_points_coordinates(TotalNumberOfPoints * 3);
        std::vector<IndexType> all_points_ids(TotalNumberOfPoints);

        // Sync all points first
        std::vector<int> all_points_ranks = SynchronizePointsWithRanks(itPointBegin, itPointEnd, all_points_coordinates, all_points_ids, rDataCommunicator, NumberOfPoints, TotalNumberOfPoints, IndexItIsJustCounter);

        // Some definitions
        IndexType i_coord = 0;

        // If distributed
        if (rDataCommunicator.IsDistributed()) {
            // Prepare MPI data
            const int rank = rDataCommunicator.Rank();
            const int world_size = rDataCommunicator.Size();
            std::vector<int> ranks(1, rank);
            std::vector<int> inside_ranks(world_size);

            // Fill actual vectors
            Point point_to_test;
            for (IndexType i_point = 0; i_point < static_cast<IndexType>(TotalNumberOfPoints); ++i_point) {
                point_to_test[0] = all_points_coordinates[3 * i_point + 0];
                point_to_test[1] = all_points_coordinates[3 * i_point + 1];
                point_to_test[2] = all_points_coordinates[3 * i_point + 2];
                const bool is_inside = PointIsInsideBoundingBox(rBoundingBox, point_to_test, ThresholdBoundingBox);
                const int search_rank = all_points_ranks[i_point];
                const bool to_be_included = is_inside || search_rank == rank;
                inside_ranks.resize(world_size);
                if (to_be_included) {
                    for (i_coord = 0; i_coord < 3; ++i_coord) {
                        rSearchInfo.PointCoordinates.push_back(all_points_coordinates[3 * i_point + i_coord]);
                    }
                    rSearchInfo.Indexes.push_back(all_points_ids[i_point]);
                    ranks[0] = rank;
                } else {
                    ranks[0] = -1;
                }
                rDataCommunicator.AllGather(ranks, inside_ranks);
                // Use std::remove_if and vector::erase to remove elements less than 0
                inside_ranks.erase(
                    std::remove_if(
                        inside_ranks.begin(),
                        inside_ranks.end(),
                        [](const int rank) { return rank < 0; }
                    ),
                    inside_ranks.end()
                );

                // Now adding // NOTE: Should be ordered, so a priori is not required to reorder
                if (to_be_included) {
                     rSearchInfo.Ranks.push_back(inside_ranks);
                     rSearchInfo.SearchRanks.push_back(search_rank);
                }
            }
        } else { // Serial
            // Assign values
            const std::size_t total_number_of_points = itPointEnd - itPointBegin;
            for (IndexType i_point = 0 ; i_point < total_number_of_points; ++i_point) {
                auto it_point = itPointBegin + i_point;
                if (PointIsInsideBoundingBox(rBoundingBox, *it_point, ThresholdBoundingBox)) {
                    const auto& r_coordinates = it_point->Coordinates();
                    for (i_coord = 0; i_coord < 3; ++i_coord) {
                        rSearchInfo.PointCoordinates.push_back(r_coordinates[i_coord]);
                    }
                    rSearchInfo.Indexes.push_back(all_points_ids[i_point]);
                    rSearchInfo.Ranks.push_back({0});
                }
            }
        }

        // Shrink to actual size
        rSearchInfo.Shrink();

        return all_points_ids;
    }

    /**
     * @brief Synchronizes the radius of all points in a distributed system.
     * @param rRecvSizes a vector containing the number of points to be received from each rank
     * @param rRadius a vector containing the radius of each point
     * @param rDataCommunicator the communication object used for data exchange
     * @return A vector containing the synchronized radius of all points
     */
    static std::vector<double> SynchronizeRadius(
        const std::vector<int>& rRecvSizes,
        const std::vector<double>& rRadius,
        const DataCommunicator& rDataCommunicator
        )
    {
        // First we calculate the total number of points to communicate
        const int total_number_of_points = std::accumulate(rRecvSizes.begin(), rRecvSizes.end(), 0);

        // Synchronize radius
        if (total_number_of_points == 0) { // If all points are the same
            return rRadius;
        } else {                           // If not
            // The resulting radius
            std::vector<double> all_points_radius(total_number_of_points);

            // MPI information
            const int world_size = rDataCommunicator.Size();

            // Generate vectors with sizes for AllGatherv
            std::vector<int> recv_offsets(world_size, 0);
            for (int i_rank = 1; i_rank < world_size; ++i_rank) {
                recv_offsets[i_rank] = recv_offsets[i_rank - 1] + rRecvSizes[i_rank - 1];
            }

            // Invoke AllGatherv
            rDataCommunicator.AllGatherv(rRadius, all_points_radius, rRecvSizes, recv_offsets);

            return all_points_radius;
        }
    }

    /**
     * @brief This method checks if all nodes are the same across all partitions
     * @param itPointBegin Iterator to the beginning of the points range
     * @param itPointEnd Iterator to the end of the points range
     * @param rNumberOfPoints Number of points in the range
     * @param rTotalNumberOfPoints Total number of points in all partitions
     * @return true if all points are the same in all partitions
     * @tparam TPointIteratorType The type of the point iterator
     */
    template<typename TPointIteratorType>
    static void CalculateNumberOfPoints(
        TPointIteratorType itPointBegin,
        TPointIteratorType itPointEnd,
        int& rNumberOfPoints,
        int& rTotalNumberOfPoints,
        const DataCommunicator& rDataCommunicator
        )
    {
        // Getting local number of points
        rNumberOfPoints = std::distance(itPointBegin, itPointEnd);

        // In case of considering nodes (and distributed)
        if (rDataCommunicator.IsDistributed()) {
            if constexpr (std::is_same<TPointIteratorType, ModelPart::NodeIterator>::value || std::is_same<TPointIteratorType, ModelPart::NodeConstantIterator>::value) {
                // Get the rank in MPI
                const int rank = rDataCommunicator.Rank();

                // Check if the nodes are local
                for (auto it_node = itPointBegin; it_node < itPointEnd; ++it_node) {
                    if (it_node->FastGetSolutionStepValue(PARTITION_INDEX) != rank) {
                        --rNumberOfPoints; // Remove is not local
                    }
                }
            }

            // Sum all the nodes
            rTotalNumberOfPoints = rDataCommunicator.SumAll(rNumberOfPoints);
        } else { // Serial
            rTotalNumberOfPoints = rNumberOfPoints;
        }
    }

    ///@}
};

} // namespace Kratos