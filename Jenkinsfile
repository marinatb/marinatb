node {

    stage 'Checkout'

        git url: 'https://github.com/marinatb/marinatb.git'
        sh 'git clean -fdx'
        sh 'git submodule init'
        sh 'git submodule update'


    stage 'Environment Setup'

        echo 'Do something'


    stage 'Build'

        sh 'mkdir build'
        sh 'cd build ; cmake .. -G Ninja'
        sh 'cd build ; ninja'


    stage 'Test Setup'

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

        sh 'docker run --privileged --hostname=builder --net=tnet -v `pwd`:/code --rm test [api-blueprint]'
        sh 'docker run --privileged --hostname=builder --net=tnet -v `pwd`:/code --rm test [api-mzn]'


    stage 'Cleanup'

        sh './builder terminate-system'
        sh 'docker network rm tnet'
}

