node {

    stage 'Checkout'

        git url: 'https://github.com/marinatb/marinatb.git'
        sh 'git clean -fdx'
        sh 'git submodule init'
        sh 'git submodule update'


    stage 'Build'

        sh 'mkdir build'
        sh 'cd build ; cmake .. -G Ninja'
        sh 'cd build ; ninja'


    stage 'Test Setup'
        env.UUID = "mtb-${env.JOB_NAME.replaceAll(/\//, '_')}-${env.BUILD_NUMBER}"

        sh './builder pkg'

        parallel (

            'containerize-system': {
                sh './builder containerize-system'
            },

            'containerize-test': {
                sh './builder containerize test'
            },

            'docker-network': {
                sh './builder net || true'
            }

        )

        sh './builder launch-system'


    stage 'Integration Tests'

        sh 'env'
        sh 'docker run --hostname=test --net=${UUID}-tnet -v "${WORKSPACE}/${JOB_NAME}":/code --rm test:${UUID} [api-blueprint]'


    stage 'Cleanup'

        sh './builder terminate-system'
        sh 'docker network rm ${UUID}-tnet'
}

