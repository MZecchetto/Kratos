import os
import json

import KratosMultiphysics.KratosUnittest as KratosUnittest
import test_helper


class KratosGeoMechanicsAbsorbingBoundaryColumnTests(KratosUnittest.TestCase):
    """
    This class contains tests which check if the lysmer absorbing boundary works in a 1d column made out of different
    element types
    """

    def setUp(self):
        self.E = 10000
        self.nu = 0.2
        self.rho = 2.65 * 0.7
        self.load = -10
        self.height_column = 10
        self.vp = None
        self.expected_velocity = None

        self.calculate_expected_p_wave_velocity()
        self.calculate_expected_velocity_column()

    def tearDown(self):
        # Code here will be placed AFTER every test in this TestCase.
        pass

    def test_absorbing_boundary_on_1d_column_quad(self):
        """
        Tests the lysmer absorbing boundary condition on a column made of rectangulars. The boundary is a 2d2n line.

        :return:
        """
        test_name = 'test_lysmer_boundary_column2d_quad.gid'
        file_path = test_helper.get_file_path(os.path.join('.', test_name))

        # quarter node, middle node, three quarter node
        node_nbrs_to_assert = [5,11,17]
        direction = 1

        self.run_and_assert_1d_column(file_path, node_nbrs_to_assert, direction)

    def test_stiff_absorbing_boundary_on_1d_column_quad(self):
        """
        Tests a very stiff Lysmer absorbing boundary condition on a column made of rectangulars. The boundary is a 2d2n
        line. Since the boundary is very stiff, the wave is completely reflected

        """

        test_name = 'test_lysmer_boundary_stiff_column2d_quad'
        file_path = test_helper.get_file_path(os.path.join('.', test_name))

        test_helper.run_kratos(file_path)

        # retrieve results from calculation
        with open(os.path.join(file_path, "calculated_result.json")) as fp:
            calculated_result = json.load(fp)

        output_indices = [0, 8, 16, 24, 32]
        calculated_velocity = [calculated_result["NODE_51"]["VELOCITY_Y"][idx] for idx in output_indices]

        # set expected results
        expected_results = [0, self.expected_velocity, 0, -self.expected_velocity, 0]

        # assert
        self.assertVectorAlmostEqual(calculated_velocity, expected_results, 2)

    def run_and_assert_1d_column(self, file_path, node_nbrs, direction):
        """
        Runs and asserts a dynamic test on a 1d column. This test checks when a p-wave arrives at a certain coordinate
        and the velocity of the corresponding node afterwards

        :param file_path: path of test
        :param node_nbrs: node nbrs to be checked
        :param direction: direction of the wave 0 = x; 1 = y; 2 = z;
        :return:

        """

        # get name of output file
        _, output_file_name = os.path.split(file_path)
        output_file_name = os.path.splitext(output_file_name)[0] + ".post.res"
        output_file_path = os.path.join(file_path,output_file_name)

        # clear old results
        if os.path.exists(output_file_path):
            os.remove(output_file_path)

        # run simulation
        simulation = test_helper.run_kratos(file_path)

        # get results from calculation
        coords = test_helper.get_nodal_coordinates(simulation)
        res = test_helper.get_nodal_variable_from_ascii(output_file_path, "DISPLACEMENT")

        # calculate and check velocity and time of arrival at all nodes
        for node_nbr in node_nbrs:
            vert_coord = coords[node_nbr-1][direction]

            # distance between top and vertical coordinate of node
            dist = self.height_column-vert_coord

            # expected time of wave arrival
            expected_ini_time = dist/self.vp

            # find index of expected time of wave arrival in time list
            res_keys = list(res.keys())
            ini_time_idx = test_helper.find_closest_index_greater_than_value(res_keys, expected_ini_time)

            # calculate velocity after wave arrival
            t1 = res_keys[ini_time_idx]
            t2 = res_keys[-1]
            dt = t2 - t1
            velocity_part_two = (res[t2][node_nbr][direction] - res[t1][node_nbr][direction])/dt

            # assert velocities
            self.assertAlmostEqual(velocity_part_two, self.expected_velocity, 2)

    def calculate_expected_p_wave_velocity(self):
        """
        Calculates the expected p_wave velocity in a 1d column
        Returns
        -------

        """
        # calculate expected p-wave velocity

        Ec = self.E * (1 - self.nu) / ((1 + self.nu) * (1 - 2 * self.nu))

        # calculate p-wave velocity
        self.vp = (Ec / self.rho) ** 0.5

    def calculate_expected_velocity_column(self):
        """
        Calculates the expected velocity in a 1d column based on the p-wave
        Returns
        -------

        """
        self.expected_velocity = self.load / (self.vp * self.rho)


if __name__ == '__main__':
    KratosUnittest.main()
